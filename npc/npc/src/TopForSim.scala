package top

import chisel3._
import chisel3.util.experimental.BoringUtils
import core._
import amba._

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

class TopForSim extends Module {
  val io = IO(new Bundle {
    val registers      = Output(Vec(16, UInt(32.W)))
    val pc             = Output(UInt(32.W))
    val dnpc           = Output(UInt(32.W))
    val inst           = Output(UInt(32.W))
    val csrs           = Output(new CSRBoring)
    val difftest_ready = Output(Bool())
  })

  implicit val p:        CoreParams  = CoreParams()
  implicit val axi_prop: AXIProperty = AXIProperty()
  val core = Module(new Core)

  core.io.interrupt    := false.B
  core.io.master.ar.ready := false.B
  core.io.master.aw.ready := false.B
  core.io.master.w.ready  := false.B
  core.io.master.r.valid  := false.B
  core.io.master.b.valid  := false.B
  core.io.master.r.bits   := 0.U.asTypeOf(core.io.master.r.bits)
  core.io.master.b.bits   := 0.U.asTypeOf(core.io.master.b.bits)

  core.io.slave.aw.valid := false.B
  core.io.slave.aw.bits  := 0.U.asTypeOf(core.io.slave.aw.bits)
  core.io.slave.w.valid  := false.B
  core.io.slave.w.bits   := 0.U.asTypeOf(core.io.slave.w.bits)
  core.io.slave.b.ready  := false.B
  core.io.slave.ar.valid := false.B
  core.io.slave.ar.bits  := 0.U.asTypeOf(core.io.slave.ar.bits)
  core.io.slave.r.ready  := false.B

  // Bore some signals for debugging
  io.registers := BoringUtils.bore(core.RegFile.regs)
  io.pc        := BoringUtils.bore(core.IFU.pc)
  io.dnpc      := BoringUtils.bore(core.WBU.dnpc)
  io.inst      := BoringUtils.bore(core.IDU.inst)

  io.csrs.mstatus   := BoringUtils.bore(core.EXU.csr_file.mstatus)
  io.csrs.mtvec     := BoringUtils.bore(core.EXU.csr_file.mtvec)
  io.csrs.mepc      := BoringUtils.bore(core.EXU.csr_file.mepc)
  io.csrs.mcause    := BoringUtils.bore(core.EXU.csr_file.mcause)
  io.csrs.mcycle    := BoringUtils.bore(core.EXU.csr_file.mcycle).asUInt(31, 0)
  io.csrs.mcycleh   := BoringUtils.bore(core.EXU.csr_file.mcycle).asUInt(63, 32)
  io.csrs.mvendorid := core.EXU.csr_file.mvendorid
  io.csrs.marchid   := core.EXU.csr_file.marchid

  // Difftest got ready after every pc advance (one instruction done),
  // which is just in.valid delayed one cycle.
  //               ___________
  //   ready      |          |
  //              _____       _____
  //   clock     |     |_____|     |_____
  //              cycle 1        cycle 2
  //                     ^
  //                     |
  //          difftest_step is called here
  io.difftest_ready := RegNext(BoringUtils.bore(core.IFU.io.in.valid))
}
