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

  val Mem = Module(new DPICMem)
  // val Mem = Module(new TempMemForSTA)

  val s_idle :: s_wait_ready :: Nil = Enum(2)

  val state = RegInit(s_idle)
  state := MuxLookup(state, s_idle)(
    Seq(
      s_idle       -> Mux(Mem.io.read_valid, s_wait_ready, s_idle),
      s_wait_ready -> Mux(io.out.ready, s_idle, s_wait_ready)
    )
  )

  val pc = RegInit(p.ResetVector.S(p.XLEN.W).asUInt)
  pc := io.in.bits.dnpc

  val inst_reg = RegInit(0.U(32.W))

  Mem.io.addr        := pc
  Mem.io.read_enable := RegNext(io.in.valid)

  Mem.io.write_enable := false.B
  Mem.io.write_mask   := 0.U
  Mem.io.write_data   := DontCare

  inst_reg := Mem.io.data_out

  io.out.bits.inst := inst_reg
  io.out.bits.pc   := pc

  io.in.ready  := state === s_idle
  io.out.valid := state === s_wait_ready
}
