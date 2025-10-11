package core

import chisel3._

import bundles.MemIO

class IFU extends Module {
  val io = IO(new Bundle {
    val pc = Input(UInt(32.W))
    val inst = Output(UInt(32.W))
  })

  // val inst = RegInit(0.U(32.W))

  // FIXME
  val Mem = Module(new DPICMem)

  Mem.io.addr := io.pc
  Mem.io.read_enable := true.B

  Mem.io.write_enable := false.B
  Mem.io.write_mask := 0.U
  Mem.io.write_data := false.B

  // inst := Mem.io.data_out
  // io.inst := inst
  io.inst := Mem.io.data_out
}