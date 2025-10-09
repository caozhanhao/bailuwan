package top

import chisel3._
import chisel3.util.experimental.BoringUtils

class sCPUTestTop(program: Seq[Byte]) extends Module {
  val io  = IO(new Bundle {
    val out      = Output(UInt(8.W))
    val outValid = Output(Bool())
    val pc       = Output(UInt(8.W))
    val regs     = Output(Vec(4, UInt(8.W)))
  })
  val cpu = Module(new sCPU(program))

  io.out      := cpu.io.out
  io.outValid := cpu.io.outValid

  io.pc   := BoringUtils.bore(cpu.pc)
  io.regs := BoringUtils.bore(cpu.regs)
}
