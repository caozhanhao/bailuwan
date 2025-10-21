package top

import chisel3._
import chisel3.util._
import core._

object StageConnect {
  def apply[T <: Data](left: DecoupledIO[T], right: DecoupledIO[T]) = {
    right <> left
  }
}

class Core(
  implicit p: CoreParams)
    extends Module {
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
  RegFile.io.rd_data         := WBU.rd_data
  RegFile.io.rd_we           := IDU.io.regfile_out.rd_we
}
