package core

import chisel3._
import chisel3.util._
import top.CoreParams

class IFUOut(
  implicit p: CoreParams)
    extends Bundle {
  val pc   = Output(UInt(p.XLEN.W))
  val inst = Output(UInt(32.W))
}

class IFU(
  implicit p: CoreParams)
    extends Module {
  val io = IO(new Bundle {
    val in  = Flipped(Decoupled(new WBUOut))
    val out = Decoupled(new IFUOut)
  })

  val mem = Module(new DPICMem)
  // val Mem = Module(new TempMemForSTA)

  val s_idle :: s_wait_mem :: s_wait_ready :: Nil = Enum(3)

  val state = RegInit(s_idle)
  state := MuxLookup(state, s_idle)(
    Seq(
      s_idle       -> Mux(mem.io.ar.ready, s_wait_mem, s_idle),
      s_wait_mem   -> Mux(mem.io.r.valid, s_wait_ready, s_wait_mem),
      s_wait_ready -> Mux(io.out.ready, s_idle, s_wait_ready)
    )
  )

  mem.io.ar.valid := state === s_idle
  mem.io.r.ready  := state === s_wait_mem

  val pc = RegInit(p.ResetVector.S(p.XLEN.W).asUInt)
  pc := Mux(io.in.valid, io.in.bits.dnpc, pc)

  mem.io.ar.bits.addr := pc
  mem.io.ar.bits.prot := 0.U

  mem.io.aw.valid := false.B
  mem.io.aw.bits  := DontCare
  mem.io.w.valid  := false.B
  mem.io.w.bits   := DontCare
  mem.io.b.ready  := false.B

  val inst_reg = RegInit(0.U(32.W))
  printf(cf"inst: ${inst_reg}, rvalid: ${mem.io.r.valid}, rdata: ${mem.io.r.bits.data}")
  inst_reg := Mux(mem.io.r.valid, mem.io.r.bits.data, inst_reg)

  io.out.bits.inst := inst_reg
  io.out.bits.pc   := pc

  io.in.ready  := io.out.ready
  io.out.valid := state === s_wait_ready
}
