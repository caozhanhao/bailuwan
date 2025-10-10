package top

import chisel3._

import core._

class Mem extends Bundle {
  val pc = Output(UInt(32.W))
  val inst = Input(UInt(32.W))
}

class Top extends Module {
  val io = IO(new Bundle {
    val mem = new Mem
  })

  val pc = RegInit(0.U(32.W))
  val inst = RegInit(0.U(32.W))

  val EXU = Module(new EXU)
  val IDU = Module(new IDU)

  IDU.io.inst := io.mem.inst
  EXU.io.decoded := IDU.io.decoded
  EXU.io.pc := pc

  pc := EXU.io.dnpc

  io.mem.pc := pc
}
