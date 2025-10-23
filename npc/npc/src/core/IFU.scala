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

  val s_idle :: s_wait_ready :: Nil = Enum(2)

  val state = RegInit(s_idle)
  state := MuxLookup(state, s_idle)(
    Seq(
      s_idle       -> Mux(mem.io.read_valid, s_wait_ready, s_idle),
      s_wait_ready -> Mux(io.out.ready, s_idle, s_wait_ready)
    )
  )

  mem.io.req_valid := state === s_idle

  val pc = RegInit(p.ResetVector.S(p.XLEN.W).asUInt)
  pc := Mux(io.in.valid, io.in.bits.dnpc, pc)

  val inst_reg = RegInit(0.U(32.W))

  mem.io.addr        := pc
  mem.io.read_enable := true.B

  mem.io.write_enable := false.B
  mem.io.write_mask   := 0.U
  mem.io.write_data   := DontCare

  inst_reg := Mux(mem.io.read_valid, mem.io.data_out, inst_reg)

  io.out.bits.inst := inst_reg
  io.out.bits.pc   := pc

  io.in.ready  := io.out.ready
  io.out.valid := state === s_wait_ready
}
