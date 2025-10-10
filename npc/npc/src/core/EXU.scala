package core

import chisel3._
import chisel3.util.MuxLookup
import bundles._

class EXU extends Module {
  val io = IO(new Bundle {
    val pc      = Input(UInt(32.W))
    val decoded = Input(new DecodedBundle)
    val dnpc    = Output(UInt(32.W))
  })

  val reg_file = Module(new RegFile)
  reg_file.io.rs1_addr := io.decoded.rs1
  reg_file.io.rs2_addr := io.decoded.rs2
  reg_file.io.rd_addr  := io.decoded.rd
  reg_file.io.rd_we    := io.decoded.rd_we

  val rs1_data = reg_file.io.rs1_data
  val rs2_data = reg_file.io.rs2_data

  val oper1 = MuxLookup(io.decoded.alu_oper1_type, 0.U)(
    Seq(
      OperType.Reg  -> rs1_data,
      OperType.Imm  -> io.decoded.imm,
      OperType.PC   -> io.pc,
      OperType.Zero -> 0.U
    )
  )

  val oper2 = MuxLookup(io.decoded.alu_oper2_type, 0.U)(
    Seq(
      OperType.Reg  -> rs2_data,
      OperType.Imm  -> io.decoded.imm,
      OperType.PC   -> io.pc,
      OperType.Zero -> 0.U
    )
  )

  // ALU
  val alu = Module(new ALU)

  alu.io.oper1  := oper1
  alu.io.oper2  := oper2
  alu.io.alu_op := io.decoded.alu_op

  // Branch
  // Default to be `pc + imm` for  beq/bne/... and jal.
  val br_target = MuxLookup(io.decoded.br_type, (io.pc + io.decoded.imm).asUInt)(
    Seq(
      BrType.None -> 0.U,
      BrType.JALR -> (reg_file.io.rs1_data + io.decoded.imm)(31, 1) ## 0.U
    )
  )

  val br_taken = MuxLookup(io.decoded.br_type, false.B)(
    Seq(
      BrType.None -> false.B,
      BrType.JAL  -> true.B,
      BrType.JALR -> true.B,
      BrType.BEQ  -> (rs1_data === rs2_data),
      BrType.BNE  -> (rs1_data =/= rs2_data),
      BrType.BLT  -> (rs1_data.asSInt < rs2_data.asSInt),
      BrType.BGE  -> (rs1_data.asSInt >= rs2_data.asSInt),
      BrType.BLTU -> (rs1_data < rs2_data),
      BrType.BGEU -> (rs1_data >= rs2_data)
    )
  )

  // Write Back
  val wbu = Module(new WBU)

  wbu.io.in.snpc      := io.pc + 4.U
  wbu.io.in.alu_out   := alu.io.result
  wbu.io.in.br_taken  := br_taken
  wbu.io.in.br_target := br_target

  reg_file.io.rd_data := wbu.io.out.rd_data
  io.dnpc             := wbu.io.out.dnpc
}
