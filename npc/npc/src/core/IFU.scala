package core

import chisel3._
import chisel3.util._
import top.CoreParams
import amba._

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

    val mem = new AXI4Lite()
  })

  val s_idle :: s_wait_mem :: s_wait_ready :: Nil = Enum(3)

  val state = RegInit(s_idle)
  state := MuxLookup(state, s_idle)(
    Seq(
      s_idle       -> Mux(io.mem.ar.fire, s_wait_mem, s_idle),
      s_wait_mem   -> Mux(io.mem.r.fire, s_wait_ready, s_wait_mem),
      s_wait_ready -> Mux(io.out.fire, s_idle, s_wait_ready)
    )
  )

  io.mem.ar.valid := state === s_idle
  io.mem.r.ready  := state === s_wait_mem

  val pc = RegInit(p.ResetVector.S(p.XLEN.W).asUInt)
  pc := Mux(io.in.fire, io.in.bits.dnpc, pc)

  io.mem.ar.bits.addr := pc
  io.mem.ar.bits.prot := 0.U

  io.mem.aw.valid := false.B
  io.mem.aw.bits  := DontCare
  io.mem.w.valid  := false.B
  io.mem.w.bits   := DontCare
  io.mem.b.ready  := false.B

  val inst_reg = RegInit(0.U(32.W))
  inst_reg := Mux(io.mem.r.fire, io.mem.r.bits.data, inst_reg)

  io.out.bits.inst := inst_reg
  io.out.bits.pc   := pc

  io.in.ready  := io.out.ready
  io.out.valid := state === s_wait_ready
}
