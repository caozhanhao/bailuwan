// Copyright (c) 2025 caozhanhao
// SPDX-License-Identifier: MIT

package bailuwan

import chisel3._
import chisel3.util._
import core._
import amba._

object PipelineConnect {
  def apply[T <: Data](
    prevOut: DecoupledIO[T],
    thisIn:  DecoupledIO[T]
  ): Unit = {
    prevOut.ready := thisIn.ready
    thisIn.bits   := RegEnable(prevOut.bits, prevOut.valid && thisIn.ready)
    thisIn.valid  := RegEnable(prevOut.valid, thisIn.ready)
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

  PipelineConnect(IFU.io.out, IDU.io.in)
  PipelineConnect(IDU.io.out, EXU.io.in)
  PipelineConnect(EXU.io.out, LSU.io.in)
  PipelineConnect(LSU.io.out, WBU.io.in)
  PipelineConnect(IFU.io.in, WBU.io.out)

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
}
