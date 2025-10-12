package core

import chisel3._

class IFU extends Module {
  val io = IO(new Bundle {
    val pc = Input(UInt(32.W))
    val inst = Output(UInt(32.W))
    val valid = Output(Bool())
  })

  val Mem = Module(new DPICMem)

  // Use a register to avoid reading invalid memory before pc inited.
  val read_enable = RegInit(false.B)
  read_enable := true.B

  Mem.io.addr := io.pc
  Mem.io.read_enable := read_enable

  Mem.io.write_enable := false.B
  Mem.io.write_mask := 0.U
  Mem.io.write_data := false.B

  io.inst := Mem.io.data_out
  io.valid := Mem.io.valid
}