package core

import chisel3._

import bundles.MemIO

class IFU extends Module {
  val io = IO(new Bundle {
    val mem_io = new MemIO
    val pc = Input(UInt(32.W))
    val inst = Output(UInt(32.W))
  })

  val mem = io.mem_io
  mem.addr := io.pc
  mem.read_enable := true.B

  io.inst := mem.data_out
}