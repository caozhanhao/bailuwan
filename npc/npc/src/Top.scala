package top

import chisel3._

import core._

class Top extends Module {
  val IFU = Module(new IFU)
  val EXU = Module(new EXU)
  val IDU = Module(new IDU)

  val Mem = Module(new DPICMem)

  val pc = RegInit(0.U(32.W))

  IFU.io.mem_io := Mem.io
  EXU.io.mem_io := Mem.io

  IDU.io.inst := IFU.io.inst
  EXU.io.decoded := IDU.io.decoded

  IFU.io.pc := pc
  EXU.io.pc := pc

  pc := EXU.io.dnpc
}
