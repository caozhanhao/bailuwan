// Copyright (c) 2025 caozhanhao
// SPDX-License-Identifier: MIT

package bailuwan

import chisel3._
import chisel3.util._
import core._
import amba._
import utils.PerfCounter

object PipelineConnect {
  def apply[T <: Data](
    prev_out:        DecoupledIO[T],
    this_in:         DecoupledIO[T],
    wbu_flush:       Bool, // WBU always force flush
    exu_flush:       Bool = false.B,
    // Force Flush indicates if we should flush the instruction stalled in register.
    exu_force_flush: Boolean = false
  ): Unit = {
    prev_out.ready := this_in.ready
    this_in.bits   := RegEnable(prev_out.bits, prev_out.valid && this_in.ready)

    val valid_reg = RegInit(false.B)

    if (exu_force_flush) {
      // Even if stalled, the data in this register is invalidated immediately.
      valid_reg := Mux(wbu_flush || exu_flush, false.B, Mux(this_in.ready, prev_out.valid, valid_reg))
    } else {
      // For non-force exu flush:
      // If stalled (!this_in.ready):  Keep `valid_reg`.
      // If not stalled, mask the input with `!flush`.
      // Imaging a state where EXU holds jal (flush=1) and LSU is Ready (No Stall).
      // At the exact rising edge of the clock:
      //   1. [EXU->LSU]: LSU captures JAL from EXU.
      //   2. [IFU->IDU]: IDU captures 0 due to force flush.
      //   3. [IDU->EXU]: EXU captures 0 due to `!flush` mask.
      //      Note that IDU currently holds the instruction after jal, and
      //    ` prev_out.valid` is still HIGH. Thus, we must mask it with `!flush`
      valid_reg := Mux(wbu_flush, false.B, Mux(this_in.ready, !exu_flush && prev_out.valid, valid_reg))
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
  val CSRFile = Module(new CSRFile)

  val exu_flush = EXU.io.br_valid
  val wbu_flush = WBU.io.redirect_valid

  PipelineConnect(IFU.io.out, IDU.io.in, wbu_flush, exu_flush, exu_force_flush = true)
  // The instruction currently in the IDU->EXU register is the jump/branch itself.
  // If the pipeline stalls, this JAL must remain in the register until it is accepted
  // by the next stage (LSU). EXU's flush MUST NOT kill the jump/branch before
  // it enters the LSU.
  PipelineConnect(IDU.io.out, EXU.io.in, wbu_flush, exu_flush, exu_force_flush = false)
  PipelineConnect(EXU.io.out, LSU.io.in, wbu_flush)
  PipelineConnect(LSU.io.out, WBU.io.in, wbu_flush)

  // Redirect
  IFU.io.redirect_valid  := exu_flush || wbu_flush
  IFU.io.redirect_target := Mux(wbu_flush, WBU.io.redirect_target, EXU.io.br_target)

  // RegFile - IDU
  IDU.io.rs1_data     := RegFile.io.rs1_data
  IDU.io.rs2_data     := RegFile.io.rs2_data
  RegFile.io.rs1_addr := IDU.io.rs1_addr
  RegFile.io.rs2_addr := IDU.io.rs2_addr

  // RegFile - WBU
  RegFile.io.rd_addr := WBU.io.rd_addr
  RegFile.io.rd_we   := WBU.io.rd_we
  RegFile.io.rd_data := WBU.io.rd_data

  // CSRFile - EXU
  EXU.io.csr_rs_data     := CSRFile.io.read_data
  CSRFile.io.read_addr   := EXU.io.csr_rs_addr
  CSRFile.io.read_enable := EXU.io.csr_rs_en

  // CSRFile - WBU
  CSRFile.io.write_addr   := WBU.io.csr_rd_addr
  CSRFile.io.write_enable := WBU.io.csr_rd_we
  CSRFile.io.write_data   := WBU.io.csr_rd_data
  CSRFile.io.exception    := WBU.io.exception
  CSRFile.io.epc          := WBU.io.csr_epc

  // ICache Flush
  IFU.io.icache_flush := EXU.io.icache_flush

  // Hazard
  IDU.io.exu_hazard_rd       := EXU.io.hazard_rd
  IDU.io.exu_hazard_rd_valid := EXU.io.hazard_rd_valid
  IDU.io.lsu_hazard_rd       := LSU.io.hazard_rd
  IDU.io.lsu_hazard_rd_valid := LSU.io.hazard_rd_valid
  IDU.io.wbu_hazard_rd       := WBU.io.rd_addr
  IDU.io.wbu_hazard_rd_valid := WBU.io.rd_we

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

  PerfCounter(true.B, "all_cycles")
}
