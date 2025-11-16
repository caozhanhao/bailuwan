package top

import chisel3._
import chisel3.util._
import core._
import amba._

object StageConnect {
  def apply[T <: Data](left: DecoupledIO[T], right: DecoupledIO[T]) = {
    right <> left
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
        (Seq((0x0200_0000L, 0xffff_ffffL))) // SoC
        // (Seq((0xa000_0048L, 0xa000_0050L)))    // MTime
      )
    )
  )
  arbiter.io.slave <> xbar.io.master

  val mtime = Module(new MTime)

  xbar.io.slaves(0) <> io.master
  xbar.io.slaves(1) <> mtime.io
}
