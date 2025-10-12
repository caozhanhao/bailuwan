package core

import chisel3._

class RegFile extends Module {
  val io = IO(new Bundle {
    val rs1_addr = Input(UInt(5.W))
    val rs2_addr = Input(UInt(5.W))

    val rd_addr = Input(UInt(5.W))
    val rd_data = Input(UInt(32.W))
    val rd_we   = Input(Bool())

    val rs1_data = Output(UInt(32.W))
    val rs2_data = Output(UInt(32.W))
  })

  // RV32E
  val regs = RegInit(VecInit(Seq.fill(16)(0.U(32.W))))

  regs(io.rd_addr(3, 0)) := Mux(io.rd_we && io.rd_addr =/= 0.U, io.rd_data, regs(io.rd_addr(3, 0)))

  io.rs1_data := regs(io.rs1_addr(3, 0))
  io.rs2_data := regs(io.rs2_addr(3, 0))
}
