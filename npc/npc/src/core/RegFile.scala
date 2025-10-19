package core

import chisel3._
import top.CoreParams

class RegFile(
  implicit p: CoreParams)
    extends Module {
  val io = IO(new Bundle {
    val rs1_addr = Input(UInt(5.W))
    val rs2_addr = Input(UInt(5.W))

    val rd_addr = Input(UInt(5.W))
    val rd_data = Input(UInt(p.XLEN.W))
    val rd_we   = Input(Bool())

    val rs1_data = Output(UInt(p.XLEN.W))
    val rs2_data = Output(UInt(p.XLEN.W))
  })

  val regs = RegInit(VecInit(Seq.fill(p.RegCount)(0.U(p.XLEN.W))))

  regs(io.rd_addr(3, 0)) := Mux(io.rd_we && io.rd_addr =/= 0.U, io.rd_data, regs(io.rd_addr(3, 0)))

  io.rs1_data := regs(io.rs1_addr(3, 0))
  io.rs2_data := regs(io.rs2_addr(3, 0))
}
