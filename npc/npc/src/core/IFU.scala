package core

import chisel3._
import chisel3.util._
import top.CoreParams
import amba._
import utils._

class IFUOut(
  implicit p: CoreParams)
    extends Bundle {
  val pc   = Output(UInt(p.XLEN.W))
  val inst = Output(UInt(32.W))
}

class IFU(
  implicit p: CoreParams,
  axi_prop:   AXIProperty)
    extends Module {
  val io = IO(new Bundle {
    val in  = Flipped(Decoupled(new WBUOut))
    val out = Decoupled(new IFUOut)

    val mem = new AXI4()
  })

  io.mem.ar.bits.id    := 0.U
  io.mem.ar.bits.len   := 0.U // burst length=1, equivalent to an AxLEN value of zero.
  io.mem.ar.bits.size  := 2.U // 2^2 = 4 bytes
  io.mem.ar.bits.burst := 0.U

  io.mem.aw.valid    := false.B
  io.mem.aw.bits     := DontCare
  io.mem.w.valid     := false.B
  io.mem.w.bits      := DontCare
  io.mem.b.ready     := false.B
  io.mem.w.bits.last := true.B

  val s_idle :: s_wait_mem :: s_wait_ready :: s_fault :: Nil = Enum(4)

  val state = RegInit(s_idle)
  state := MuxLookup(state, s_idle)(
    Seq(
      s_idle       -> Mux(io.mem.ar.fire, s_wait_mem, s_idle),
      s_wait_mem   -> Mux(io.mem.r.fire, Mux(io.mem.r.bits.resp === AXIResp.OKAY, s_wait_ready, s_fault), s_wait_mem),
      s_wait_ready -> Mux(io.out.fire, s_idle, s_wait_ready)
    )
  )

  io.mem.ar.valid := (state === s_idle) && !reset.asBool // Don't send request when resetting
  io.mem.r.ready  := state === s_wait_mem

  val pc = RegInit(p.ResetVector.S(p.XLEN.W).asUInt)
  pc := Mux(io.in.fire, io.in.bits.dnpc, pc)

  io.mem.ar.bits.addr := pc

  val inst_reg = RegInit(0.U(32.W))
  inst_reg := Mux(io.mem.r.fire, io.mem.r.bits.data, inst_reg)

  io.out.bits.inst := inst_reg
  io.out.bits.pc   := pc

  io.in.ready  := io.out.ready
  io.out.valid := state === s_wait_ready

  val fault_addr = RegInit(0.U(p.XLEN.W))
  fault_addr := Mux(io.mem.ar.fire, pc, fault_addr)
  val fault_resp = RegInit(AXIResp.OKAY)
  fault_resp := Mux(io.mem.r.fire, io.mem.r.bits.resp, fault_resp)

  assert(state =/= s_fault, cf"IFU: Access fault at 0x${fault_addr}%x, resp=${fault_resp}")

  if (p.Debug) {
    // Difftest got ready after every pc advance (one instruction done),
    // which is just in.valid delayed one cycle.
    //               ___________
    //   ready      |          |
    //              _____       _____
    //   clock     |     |_____|     |_____
    //              cycle 1        cycle 2
    //                     ^
    //                     |
    //          difftest_step is called here
    val difftest_ready = RegNext((false.B ## io.in.valid))
    // An assert to avoid optimization by Verilator when trace is disabled.
    assert(!difftest_ready(1))
    // A dontTouch to avoid optimization by Chisel.
    dontTouch(difftest_ready)

    // val fetched_cnt = PerfCounter(io.mem.r.fire)
    val perf = Module(new PerfCounter)
    perf.io.cond := io.mem.r.fire
    val fetched_cnt = perf.io.out
    dontTouch(fetched_cnt)
  }
}
