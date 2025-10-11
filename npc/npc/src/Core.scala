package top

import chisel3._
import core._

class Core extends Module {
  val IFU = Module(new IFU)
  val EXU = Module(new EXU)
  val IDU = Module(new IDU)

  val pc = RegInit(0.U(32.W))

  IDU.io.inst    := IFU.io.inst
  IDU.io.inst_valid := IFU.io.valid
  EXU.io.decoded := IDU.io.decoded

  IFU.io.pc := pc
  EXU.io.pc := pc

  pc := EXU.io.dnpc
}
