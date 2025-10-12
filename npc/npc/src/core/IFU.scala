package core

import chisel3._

class IFU extends Module {
  val io = IO(new Bundle {
    val pc   = Input(UInt(32.W))
    val inst = Output(UInt(32.W))
  })

  val Mem = Module(new DPICMem)

  Mem.io.addr        := io.pc
  Mem.io.read_enable := true.B

  Mem.io.write_enable := false.B
  Mem.io.write_mask   := 0.U
  Mem.io.write_data   := false.B

  val NOP = 0x00000013.U(32.W)
  io.inst := Mux(Mem.io.valid, Mem.io.data_out, NOP)
}
