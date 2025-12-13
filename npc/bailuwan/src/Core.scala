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
    prevOut: DecoupledIO[T],
    thisIn:  DecoupledIO[T],
    flush:   Bool = false.B
  ): Unit = {
    prevOut.ready := thisIn.ready
    thisIn.bits   := RegEnable(prevOut.bits, prevOut.valid && thisIn.ready)

    val valid_reg = RegInit(false.B)
    valid_reg    := Mux(flush, false.B, Mux(thisIn.ready, prevOut.valid, valid_reg))
    thisIn.valid := valid_reg
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

  // Note that redirect should flush the instruction in IFU -> IDU,
  // but MUST NOT flush the instruction already in IDU -> EXU.
  // The instruction in IDU -> EXU is the branch/jump itself and must complete
  // the rest of the pipeline (for example, `jalr` must reach WBU to update rd).
  PipelineConnect(IFU.io.out, IDU.io.in, EXU.io.redirect_valid)
  PipelineConnect(IDU.io.out, EXU.io.in)
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

  PerfCounter(true.B, "all_cycles")
}
