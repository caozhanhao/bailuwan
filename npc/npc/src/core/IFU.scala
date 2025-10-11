package core

import chisel3._

import bundles.MemIO

class IFU extends Module {
  val io = IO(new Bundle {
    val mem_io = new MemIO
    val pc = Input(UInt(32.W))
    val inst = Output(UInt(32.W))
  })

  io.mem_io.addr := io.pc
  io.mem_io.read_enable := true.B

  io.inst := io.mem_io.data_out
}