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

class DebugBoringBundle extends Bundle {
  val registers      = Vec(16, UInt(32.W))
  val pc             = UInt(32.W)
  val dnpc           = UInt(32.W)
  val inst           = UInt(32.W)
  val csrs           = new CSRBoring
  val difftest_ready = Bool()
}

class TopForSoC extends Module {
  override val desiredName = "ysyx_25100251"

  implicit val p:        CoreParams  = CoreParams()
  implicit val axi_prop: AXIProperty = AXIProperty()

  val io = IO(new Bundle {
    val master    = new AXI4
    val slave     = Flipped(new AXI4)
    val interrupt = Input(Bool())
    val dbg       = Output(new DebugBoringBundle)
  })

  val core = Module(new Core)
  core.io <> io

  // Bore some signals for debugging
  io.dbg.registers := BoringUtils.bore(core.RegFile.regs)
  io.dbg.pc        := BoringUtils.bore(core.IFU.pc)
  io.dbg.dnpc      := BoringUtils.bore(core.WBU.dnpc)
  io.dbg.inst      := BoringUtils.bore(core.IDU.inst)

  io.dbg.csrs.mstatus   := BoringUtils.bore(core.EXU.csr_file.mstatus)
  io.dbg.csrs.mtvec     := BoringUtils.bore(core.EXU.csr_file.mtvec)
  io.dbg.csrs.mepc      := BoringUtils.bore(core.EXU.csr_file.mepc)
  io.dbg.csrs.mcause    := BoringUtils.bore(core.EXU.csr_file.mcause)
  io.dbg.csrs.mcycle    := BoringUtils.bore(core.EXU.csr_file.mcycle).asUInt(31, 0)
  io.dbg.csrs.mcycleh   := BoringUtils.bore(core.EXU.csr_file.mcycle).asUInt(63, 32)
  io.dbg.csrs.mvendorid := core.EXU.csr_file.mvendorid
  io.dbg.csrs.marchid   := core.EXU.csr_file.marchid

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
  io.dbg.difftest_ready := RegNext(BoringUtils.bore(core.IFU.io.in.valid))
}
