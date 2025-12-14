// Copyright (c) 2025 caozhanhao
// SPDX-License-Identifier: MIT

package bailuwan

import chisel3._
import chisel3.util._
import core._
import amba._
import constants.{BrOp, ExecType}
import utils.{PerfCounter, SignalProbe}

object PipelineConnect {
  def apply[T <: Data](
    prev_out:    DecoupledIO[T],
    this_in:     DecoupledIO[T],
    flush:       Bool = false.B,
    // Force Flush indicates if we should flush the instruction stalled in register.
    force_flush: Boolean = false
  ): Unit = {
    prev_out.ready := this_in.ready
    this_in.bits   := RegEnable(prev_out.bits, prev_out.valid && this_in.ready)

    val valid_reg = RegInit(false.B)

    if (force_flush) {
      // Even if stalled, the data in this register is invalidated immediately.
      valid_reg := Mux(flush, false.B, Mux(this_in.ready, prev_out.valid, valid_reg))
    } else {
      // If stalled (!this_in.ready):  Keep `valid_reg`.
      // If not stalled, mask the input with `!flush`.
      // Imaging a state where EXU holds jal (flush=1) and LSU is Ready (No Stall).
      // At the exact rising edge of the clock:
      //   1. [EXU->LSU]: LSU captures JAL from EXU.
      //   2. [IFU->IDU]: IDU captures 0 due to force flush.
      //   3. [IDU->EXU]: EXU captures 0 due to `!flush` mask.
      //      Note that IDU currently holds the instruction after jal, and
      //    ` prev_out.valid` is still HIGH. Thus, we must mask it with `!flush`
      valid_reg := Mux(this_in.ready, !flush && prev_out.valid, valid_reg)
    }

    this_in.valid := valid_reg
  }
}

class Core(
  implicit p: CoreParams,
  axi_prop:   AXIProperty)
    extends Module {

  val io = IO(new Bundle {
    val master    = new AXI4
    val slave     = Flipped(new AXI4)
    val interrupt = Input(Bool())
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
  val IDU = Module(new IDU)
  val EXU = Module(new EXU)
  val LSU = Module(new LSU)
  val WBU = Module(new WBU)

  val RegFile = Module(new RegFile)

  val flush = EXU.io.redirect_valid

  // The instruction currently in the IFU -> IDU register is possibly on the
  // wrong branch (or follows a jump). We must clear it immediately, even if IDU is stalled.
  PipelineConnect(IFU.io.out, IDU.io.in, flush, force_flush = true)
  // The instruction currently in the IDU->EXU register is the jump/branch itself.
  // If the pipeline stalls, this JAL must remain in the register until it is accepted
  // by the next stage (LSU). A force flush would kill the jump/branch before it enters the LSU.
  PipelineConnect(IDU.io.out, EXU.io.in, flush, force_flush = false)
  PipelineConnect(EXU.io.out, LSU.io.in)
  PipelineConnect(LSU.io.out, WBU.io.in)

  // Redirect
  IFU.io.redirect_valid  := EXU.io.redirect_valid
  IFU.io.redirect_target := EXU.io.redirect_target

  // Regfile - IDU
  IDU.io.regfile_in.rs1_data := RegFile.io.rs1_data
  IDU.io.regfile_in.rs2_data := RegFile.io.rs2_data
  RegFile.io.rs1_addr        := IDU.io.regfile_out.rs1_addr
  RegFile.io.rs2_addr        := IDU.io.regfile_out.rs2_addr
  // Regfile - WBU
  RegFile.io.rd_addr         := WBU.io.regfile_out.rd_addr
  RegFile.io.rd_we           := WBU.io.regfile_out.rd_we
  RegFile.io.rd_data         := WBU.io.regfile_out.rd_data

  // ICache Flush
  IFU.io.icache_flush := EXU.io.icache_flush

  // Hazard
  IDU.io.exu_rd       := EXU.io.rd
  IDU.io.exu_rd_valid := EXU.io.rd_valid
  IDU.io.lsu_rd       := LSU.io.rd
  IDU.io.lsu_rd_valid := LSU.io.rd_valid
  IDU.io.wbu_rd       := WBU.io.regfile_out.rd_addr
  IDU.io.wbu_rd_valid := WBU.io.regfile_out.rd_we

  // Memory, LSU > IFU
  val arbiter = Module(new AXI4Arbiter(2))
  arbiter.io.masters(0) <> LSU.io.mem
  arbiter.io.masters(1) <> IFU.io.mem

  val xbar = Module(
    new AXI4CrossBar(
      Seq(
        // SoC
        Seq(
          (0x0f000000L, 0x0fffffffL), // SRAM
          (0x10000000L, 0x10000fffL), // UART16550
          (0x10001000L, 0x10001fffL), // SPI master
          (0x10002000L, 0x1000200fL), // GPIO
          (0x10011000L, 0x10011007L), // PS2
          (0x20000000L, 0x20000fffL), // MROM
          (0x21000000L, 0x211fffffL), // VGA
          (0x30000000L, 0x3fffffffL), // FLASH
          (0x40000000L, 0x7fffffffL), // ChipLink MMIO
          (0x80000000L, 0x9fffffffL), // PSRAM
          (0xa0000000L, 0xbfffffffL), // SDRAM
          (0xc0000000L, 0xffffffffL)  // ChipLink MEM
        ),
        // CLINT
        Seq((0x02000000L, 0x0200ffffL))
      )
    )
  )

  val clint = Module(new CLINT())
  xbar.io.slaves(0) <> io.master
  xbar.io.slaves(1) <> clint.io

  arbiter.io.slave <> xbar.io.master

  // Perf Counters
  import chisel3.util.BitPat

  case class InstTypeProbe(
    is_alu: Bool,
    is_br:  Bool,
    is_lsu: Bool,
    is_csr: Bool)

  def QuickDecode(inst: UInt): InstTypeProbe = {
    val opcode = inst(6, 0)

    val is_load   = opcode === BitPat("b0000011")
    val is_store  = opcode === BitPat("b0100011")
    val is_branch = opcode === BitPat("b1100011")
    val is_jal    = opcode === BitPat("b1101111")
    val is_jalr   = opcode === BitPat("b1100111")
    val is_lui    = opcode === BitPat("b0110111")
    val is_auipc  = opcode === BitPat("b0010111")
    val is_op_imm = opcode === BitPat("b0010011")
    val is_op     = opcode === BitPat("b0110011")
    val is_csr    = opcode === BitPat("b1110011")

    val is_lsu_type = is_load || is_store
    val is_br_type  = is_branch || is_jal || is_jalr
    val is_csr_type = is_csr
    val is_alu_type = is_lui || is_auipc || is_op_imm || is_op

    InstTypeProbe(is_alu_type, is_br_type, is_lsu_type, is_csr_type)
  }

  case class StageInfo(valid: Bool, inst: UInt)
  val stages = Seq(
    StageInfo(IDU.io.in.valid, IDU.io.in.bits.inst),
    StageInfo(EXU.io.in.valid, EXU.io.in.bits.inst),
    StageInfo(LSU.io.in.valid, LSU.io.in.bits.lsu.inst),
    StageInfo(WBU.io.in.valid, WBU.io.in.bits.inst)
  )

  def countCycle(selector: InstTypeProbe => Bool): UInt = {
    stages.map { stage =>
      val info = QuickDecode(stage.inst)
      (stage.valid && selector(info)).asUInt
    }.reduce(_ +& _)
  }

  val alu_cycles = countCycle(_.is_alu)
  val br_cycles  = countCycle(_.is_br)
  val lsu_cycles = countCycle(_.is_lsu)
  val csr_cycles = countCycle(_.is_csr)

  val all_cycles = stages.map(_.valid.asUInt).reduce(_ +& _)

  val other_cycles = all_cycles - (alu_cycles + br_cycles + lsu_cycles + csr_cycles)

  def CycleCounter(cycles: UInt, name: String): Unit = {
    val counter = RegInit(0.U(64.W))
    counter := counter + cycles
    SignalProbe(counter, name)
  }

  CycleCounter(all_cycles, "all_cycles")
  CycleCounter(alu_cycles, "alu_cycles")
  CycleCounter(br_cycles, "br_cycles")
  CycleCounter(lsu_cycles, "lsu_cycles")
  CycleCounter(csr_cycles, "csr_cycles")
  CycleCounter(other_cycles, "other_cycles")
}
