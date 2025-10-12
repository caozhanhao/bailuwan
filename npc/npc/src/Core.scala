package top

import chisel3._
import core._

class Core extends Module {
  val IFU = Module(new IFU)
  val EXU = Module(new EXU)
  val IDU = Module(new IDU)

  val pc = RegInit(0x80000000.S(32.W).asUInt)

  IDU.io.inst       := IFU.io.inst
  EXU.io.decoded    := IDU.io.decoded

  IFU.io.pc := pc
  EXU.io.pc := pc

  pc := EXU.io.dnpc
}
