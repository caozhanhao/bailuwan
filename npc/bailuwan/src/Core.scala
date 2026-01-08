// Copyright (c) 2025-2026 caozhanhao
// SPDX-License-Identifier: MulanPSL-2.0

package bailuwan

import chisel3._
import chisel3.util._
import core._
import amba._
import utils.PerfCounter

// invalidate: Forces the valid bit to 0 in the next cycle.
//             Thus, the data currently in this register is immediately flushed.
// inject_bubble: Let a bubble (valid=0) enters this stage instead of the upstream data.
//                Unlike invalidate, this only acts when the pipeline advances (ready=1).
//                Thus, the data currently in this register is preserved if the pipeline stalls.
object PipelineConnect {
  def apply[T <: Data](
    prev_out:      DecoupledIO[T],
    this_in:       DecoupledIO[T],
    invalidate:    Bool = false.B,
    inject_bubble: Bool = false.B
  ): Unit = {
    prev_out.ready := this_in.ready
    this_in.bits   := RegEnable(prev_out.bits, prev_out.valid && this_in.ready)

    val valid_reg = RegInit(false.B)
    valid_reg := Mux(invalidate, false.B, Mux(this_in.ready, !inject_bubble && prev_out.valid, valid_reg))

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

  val exu_flush = EXU.io.br_mispredict
  val wbu_flush = WBU.io.redirect_valid

  // IFU -> IDU:
  // Flush on either branch mispredict (EXU) or exception/redirect (WBU).
  PipelineConnect(IFU.io.out, IDU.io.in, invalidate = wbu_flush || exu_flush)

  // IDU -> EXU:
  // Note that for [IDU->EXU], exu_flush should not flush that instruction immediately.
  // Because the instruction currently in the [IDU->EXU] register is the jump/branch itself.
  // If the pipeline stalls, this JAL must remain in the register until it is accepted
  // by the next stage (LSU).
  //
  // Besides, `inject_bubble` ensures that when the pipeline advances, the EXU accepts
  // a bubble instead of the wrong instruction from IDU.
  // Imaging a state where EXU holds jal (flush=1) and LSU is Ready (No Stall).
  // And wbu_flush = false, exu_flush = true
  // At the exact rising edge of the clock:
  //   1. [EXU->LSU]: LSU captures JAL from EXU.
  //   2. [IDU->EXU]: EXU captures 0 due to inject_bubble.
  //      Note that IDU currently holds the wrong instruction after jal, and
  //    ` prev_out.valid` is still HIGH. Thus, we must inject a bubble in EXU.
  //   3. [IFU->IDU]: IDU captures 0 due to invalidate by exu_flush.
  PipelineConnect(IDU.io.out, EXU.io.in, invalidate = wbu_flush, inject_bubble = exu_flush)

  // EXU -> LSU, LSU -> WBU:
  // Flush only on exception/redirect (WBU).
  PipelineConnect(EXU.io.out, LSU.io.in, invalidate = wbu_flush)
  PipelineConnect(LSU.io.out, WBU.io.in, invalidate = wbu_flush)

  // Redirect
  IFU.io.redirect_valid  := exu_flush || wbu_flush
  IFU.io.redirect_target := Mux(wbu_flush, WBU.io.redirect_target, EXU.io.correct_target)
  LSU.io.wbu_flush       := wbu_flush

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
  WBU.io.mtvec            := CSRFile.io.mtvec
  WBU.io.mepc             := CSRFile.io.mepc
  CSRFile.io.write_addr   := WBU.io.csr_rd_addr
  CSRFile.io.write_enable := WBU.io.csr_rd_we
  CSRFile.io.write_data   := WBU.io.csr_rd_data
  CSRFile.io.exception    := WBU.io.exception
  CSRFile.io.epc          := WBU.io.csr_epc

  // LSU Flush
  LSU.io.wbu_flush := wbu_flush

  // ICache Flush
  IFU.io.icache_flush := WBU.io.icache_flush

  // BPU
  IFU.io.btb_w := EXU.io.btb_w

  // Hazard
  IDU.io.exu_hazard := EXU.io.hazard
  IDU.io.lsu_hazard := LSU.io.hazard
  IDU.io.wbu_hazard := WBU.io.hazard

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
