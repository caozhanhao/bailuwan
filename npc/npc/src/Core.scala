package top

import chisel3._
import chisel3.util._
import chisel3.util.experimental.BoringUtils
import core._
import amba._

object StageConnect {
  def apply[T <: Data](left: DecoupledIO[T], right: DecoupledIO[T]) = {
    right <> left
  }
}

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

class BoringBundle extends Bundle {
  val registers      = Vec(16, UInt(32.W))
  val pc             = UInt(32.W)
  val dnpc           = UInt(32.W)
  val inst           = UInt(32.W)
  val csrs           = new CSRBoring
  val difftest_ready = Bool()
}

class Core(
  implicit p: CoreParams,
  axi_prop:   AXIProperty)
    extends Module {

  val io = IO(new Bundle {
    val master    = new AXI4
    val slave     = Flipped(new AXI4)
    val interrupt = Input(Bool())

    // Debug
    val dbg = new BoringBundle
  })

  io.master.aw.valid := false.B
  io.master.aw.bits  := 0.U.asTypeOf(io.master.aw.bits)
  io.master.w.valid  := false.B
  io.master.w.bits   := 0.U.asTypeOf(io.master.w.bits)
  io.master.b.ready  := false.B
  io.master.ar.valid := false.B
  io.master.ar.bits  := 0.U.asTypeOf(io.master.ar.bits)
  io.master.r.ready  := false.B

  io.slave.ar.ready := false.B
  io.slave.aw.ready := false.B
  io.slave.w.ready  := false.B
  io.slave.r.valid  := false.B
  io.slave.b.valid  := false.B
  io.slave.r.bits   := 0.U.asTypeOf(io.slave.r.bits)
  io.slave.b.bits   := 0.U.asTypeOf(io.slave.b.bits)

  val IFU = Module(new IFU)
  val EXU = Module(new EXU)
  val IDU = Module(new IDU)
  val WBU = Module(new WBU)

  val RegFile = Module(new RegFile)

  // Stage Connect
  StageConnect(IFU.io.out, IDU.io.in)
  StageConnect(IDU.io.out, EXU.io.in)
  StageConnect(EXU.io.out, WBU.io.in)
  StageConnect(WBU.io.out, IFU.io.in)

  // Regfile
  IDU.io.regfile_in.rs1_data := RegFile.io.rs1_data
  IDU.io.regfile_in.rs2_data := RegFile.io.rs2_data
  RegFile.io.rs1_addr        := IDU.io.regfile_out.rs1_addr
  RegFile.io.rs2_addr        := IDU.io.regfile_out.rs2_addr
  RegFile.io.rd_addr         := IDU.io.regfile_out.rd_addr
  RegFile.io.rd_we           := WBU.io.regfile_out.rd_we
  RegFile.io.rd_data         := WBU.io.regfile_out.rd_data

  // Memory
  val arbiter = Module(new AXI4Arbiter(2))
  arbiter.io.masters(0) <> IFU.io.mem
  arbiter.io.masters(1) <> EXU.io.mem

  arbiter.io.slave <> io.master

  val xbar = Module(
    new AXI4CrossBar(
      Seq(
        (Seq((0x0000_0000L, 0xa000_0048L), (0xa000_0050L, 0xffff_ffffL))), // SoC
        (Seq((0xa000_0048L, 0xa000_0050L)))                                // MTime
      )
    )
  )
  arbiter.io.slave <> xbar.io.master

  val mtime = Module(new MTime)

  xbar.io.slaves(0) <> io.master
  xbar.io.slaves(1) <> mtime.io

  // Bore some signals for debugging
  io.dbg.registers := BoringUtils.bore(RegFile.regs)
  io.dbg.pc        := BoringUtils.bore(IFU.pc)
  io.dbg.dnpc      := BoringUtils.bore(WBU.dnpc)
  io.dbg.inst      := BoringUtils.bore(IDU.inst)

  io.dbg.csrs.mstatus   := BoringUtils.bore(EXU.csr_file.mstatus)
  io.dbg.csrs.mtvec     := BoringUtils.bore(EXU.csr_file.mtvec)
  io.dbg.csrs.mepc      := BoringUtils.bore(EXU.csr_file.mepc)
  io.dbg.csrs.mcause    := BoringUtils.bore(EXU.csr_file.mcause)
  io.dbg.csrs.mcycle    := BoringUtils.bore(EXU.csr_file.mcycle).asUInt(31, 0)
  io.dbg.csrs.mcycleh   := BoringUtils.bore(EXU.csr_file.mcycle).asUInt(63, 32)
  io.dbg.csrs.mvendorid := EXU.csr_file.mvendorid
  io.dbg.csrs.marchid   := EXU.csr_file.marchid

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
  io.dbg.difftest_ready := RegNext(BoringUtils.bore(IFU.io.in.valid))
}
