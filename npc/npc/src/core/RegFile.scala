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

  assert(
    io.rd_addr < 16.U && io.rs1_addr < 16.U && io.rs2_addr < 16.U,
    s"Not RV32E Inst?(rs1: ${io.rs1_addr}, rs2: ${io.rs2_addr}, rd: ${io.rd_addr})"
  )

  // RV32E
  val regs = RegInit(VecInit(Seq.fill(16)(0.U(32.W))))

  regs(io.rd_addr(3, 0)) := Mux(io.rd_we && io.rd_addr =/= 0.U, io.rd_data, regs(io.rd_addr))

  io.rs1_data := regs(io.rs1_addr(3, 0))
  io.rs2_data := regs(io.rs2_addr(3, 0))
}
