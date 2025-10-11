package core

import chisel3._
import chisel3.util._
import constants._

class WriteBackIn extends Bundle {
  // Register File
  val src_type = UInt(ExecType.WIDTH)
  val alu_out  = UInt(32.W)
  val lsu_out  = UInt(32.W)

  // PC
  val snpc      = UInt(32.W)
  val br_taken  = Bool()
  val br_target = UInt(32.W)
}

class WriteBackOut extends Bundle {
  // Register File
  val rd_data = UInt(32.W)

  // PC
  val dnpc = UInt(32.W)
}


class WBU extends Module {
  val io = IO(new Bundle {
    val in  = Input(new WriteBackIn)
    val out = Output(new WriteBackOut)
  })

  val dnpc = Mux(io.in.br_taken, io.in.br_target, io.in.snpc)

  import ExecType._
  val rd_data = MuxLookup(io.in.src_type, 0.U)(Seq(
    ALU -> io.in.alu_out,
    LSU -> io.in.lsu_out,
  ))

  io.out.dnpc := dnpc
  io.out.rd_data := rd_data
}
