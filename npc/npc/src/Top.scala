package top

import chisel3._
import chisel3.util.experimental.BoringUtils
import core._

class CSRBoring extends Bundle {
  val mstatus   = UInt(32.W)
  val mtvec     = UInt(32.W)
  val mepc      = UInt(32.W)
  val mcause    = UInt(32.W)
  val mcycle    = UInt(32.W)
  val mcycleh   = UInt(32.W)
  val mvendorid = UInt(32.W)
  val marchid   = UInt(32.W)
}

class Top extends Module {
  val io = IO(new Bundle {
    val registers = Output(Vec(16, UInt(32.W)))
    val pc        = Output(UInt(32.W))
    val dnpc      = Output(UInt(32.W))
    val inst      = Output(UInt(32.W))
    val csrs      = Output(new CSRBoring)
  })

  implicit val p: CoreParams = CoreParams()
  val core = Module(new Core)

  // Bore some signals for debugging
  io.registers := BoringUtils.bore(core.EXU.reg_file.regs)
  io.pc        := BoringUtils.bore(core.pc)
  io.dnpc      := BoringUtils.bore(core.EXU.wbu.dnpc)
  io.inst      := BoringUtils.bore(core.IFU.io.inst)

  io.csrs.mstatus   := BoringUtils.bore(core.EXU.csr_file.mstatus)
  io.csrs.mtvec     := BoringUtils.bore(core.EXU.csr_file.mtvec)
  io.csrs.mepc      := BoringUtils.bore(core.EXU.csr_file.mepc)
  io.csrs.mcause    := BoringUtils.bore(core.EXU.csr_file.mcause)
  io.csrs.mcycle    := BoringUtils.bore(core.EXU.csr_file.mcycle)
  io.csrs.mcycleh   := BoringUtils.bore(core.EXU.csr_file.mcycleh)
  io.csrs.mvendorid := 0.U // BoringUtils.bore(core.EXU.csr_file.mvendorid)
  io.csrs.marchid   := 0.U // BoringUtils.bore(core.EXU.csr_file.marchid)
}
