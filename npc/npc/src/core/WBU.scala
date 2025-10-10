package core

import chisel3._
import chisel3.util._
import bundles._

class WBU extends Module {
  val io = IO(new Bundle {
    val in  = Input(new WriteBackIn)
    val out = Output(new WriteBackOut)
  })

  val dnpc = Mux(io.in.br_taken, io.in.br_target, io.in.snpc)

  import WriteBackSource._
  val rd_data = MuxLookup(io.in.src_type, 0.U)(Seq(
    ALU -> io.in.alu_out,
    LSU -> io.in.lsu_out,
  ))

  io.out.dnpc := dnpc
  io.out.rd_data := rd_data
}
