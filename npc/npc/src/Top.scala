package top

import chisel3._
import chisel3.util.experimental.BoringUtils
import core._

class Top extends Module {
  val io = IO(new Bundle {
    val registers = Output(Vec(16, UInt(32.W)))
  })

  val IFU = Module(new IFU)
  val EXU = Module(new EXU)
  val IDU = Module(new IDU)

  val pc = RegInit(0.U(32.W))

  IDU.io.inst := IFU.io.inst
  EXU.io.decoded := IDU.io.decoded

  IFU.io.pc := pc
  EXU.io.pc := pc

  pc := EXU.io.dnpc
  io.registers := BoringUtils.bore(EXU.reg_file.regs)
}
