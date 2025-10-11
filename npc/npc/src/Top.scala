package top

import chisel3._
import chisel3.util.experimental.BoringUtils
import core._

class Top extends Module {
  val io = IO(new Bundle {
    val registers = Output(Vec(16, UInt(32.W)))
    val pc        = Output(UInt(32.W))
    val inst      = Output(UInt(32.W))
  })

  val core = Module(new Core)

  // Bore some signals for debugging
  io.registers := BoringUtils.bore(core.EXU.reg_file.regs)
  io.pc        := BoringUtils.bore(core.pc)
  io.inst      := BoringUtils.bore(core.IFU.io.inst)
}
