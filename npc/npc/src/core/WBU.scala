package core

import chisel3._
import chisel3.util._
import constants._
import top.CoreParams

class WBUOut(
  implicit p: CoreParams)
    extends Bundle {
  // PC
  val dnpc = UInt(p.XLEN.W)
}

class WBURegfileOut(
  implicit p: CoreParams)
    extends Bundle {
  val rd_data = UInt(p.XLEN.W)
}

class WBU(
  implicit p: CoreParams)
    extends Module {
  val io = IO(new Bundle {
    val in  = Flipped(Decoupled(new EXUOut))
    val out = Decoupled(new WBUOut)

    val regfile_out = Output(new WBURegfileOut)
  })

  val exu_out = io.in.bits

  val br_dnpc = Mux(exu_out.br_taken, exu_out.br_target, exu_out.snpc)
  val dnpc    = MuxLookup(exu_out.src_type, br_dnpc)(
    Seq(
      ExecType.ECall -> exu_out.csr_out,
      ExecType.MRet  -> exu_out.csr_out
    )
  )

  import ExecType._
  val rd_data = MuxLookup(exu_out.src_type, 0.U)(
    Seq(
      ALU -> exu_out.alu_out,
      LSU -> exu_out.lsu_out,
      CSR -> exu_out.csr_out
    )
  )

  io.out.bits.dnpc       := dnpc
  io.regfile_out.rd_data := rd_data

  // io.in.ready  := io.out.ready
  // io.out.valid := io.in.valid
  io.in.ready := true.B
  io.out.valid := true.B
}
