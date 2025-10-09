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

  val ALU = Module(new ALU)
  val EXU = Module(new EXU)
  val IDU = Module(new IDU)
  val IFU = Module(new IFU)
  val LSU = Module(new LSU)
  val WBU = Module(new WBU)

  io.mem.pc := pc
  inst := IFU.io.inst

  IDU.io.inst := inst
  EXU.io.decoded := IDU.io.decoded
}
