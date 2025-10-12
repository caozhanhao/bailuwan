package core

import chisel3._
import chisel3.util._
import constants._

import utils.Utils._

object InstDecodeTable {
  import InstPat._
  import InstFmt._
  import OperType._
  import ExecType._

  val T = true.B
  val F = false.B

  // format, oper1, oper2, WE, ALUOp, BrOp, LSUOp, ExecType
  val default = List(Err, Zero, Zero, T, ALUOp.Add, BrOp.None, LSUOp.None, ALU)
  val table   = Array(
    LUI    -> List(U, Imm, Zero, T, ALUOp.Add, BrOp.None, LSUOp.None, ALU),
    AUIPC  -> List(U, PC, Imm, T, ALUOp.Add, BrOp.None, LSUOp.None, ALU),
    JAL    -> List(J, PC, Four, T, ALUOp.Add, BrOp.JAL, LSUOp.None, ALU),
    JALR   -> List(I, PC, Four, T, ALUOp.Add, BrOp.JALR, LSUOp.None, ALU),
    BEQ    -> List(B, Zero, Zero, F, ALUOp.Add, BrOp.BEQ, LSUOp.None, ALU),
    BNE    -> List(B, Zero, Zero, F, ALUOp.Add, BrOp.BNE, LSUOp.None, ALU),
    BLT    -> List(B, Zero, Zero, F, ALUOp.Add, BrOp.BLT, LSUOp.None, ALU),
    BGE    -> List(B, Zero, Zero, F, ALUOp.Add, BrOp.BGE, LSUOp.None, ALU),
    BLTU   -> List(B, Zero, Zero, F, ALUOp.Add, BrOp.BLTU, LSUOp.None, ALU),
    BGEU   -> List(B, Zero, Zero, F, ALUOp.Add, BrOp.BGEU, LSUOp.None, ALU),
    LB     -> List(I, Rs1, Imm, T, ALUOp.Add, BrOp.None, LSUOp.LB, LSU),
    LH     -> List(I, Rs1, Imm, T, ALUOp.Add, BrOp.None, LSUOp.LH, LSU),
    LW     -> List(I, Rs1, Imm, T, ALUOp.Add, BrOp.None, LSUOp.LW, LSU),
    LBU    -> List(I, Rs1, Imm, T, ALUOp.Add, BrOp.None, LSUOp.LBU, LSU),
    LHU    -> List(I, Rs1, Imm, T, ALUOp.Add, BrOp.None, LSUOp.LHU, LSU),
    SB     -> List(S, Rs1, Imm, F, ALUOp.Add, BrOp.None, LSUOp.SB, LSU),
    SH     -> List(S, Rs1, Imm, F, ALUOp.Add, BrOp.None, LSUOp.SH, LSU),
    SW     -> List(S, Rs1, Imm, F, ALUOp.Add, BrOp.None, LSUOp.SW, LSU),
    ADDI   -> List(I, Rs1, Imm, T, ALUOp.Add, BrOp.None, LSUOp.None, ALU),
    SLTI   -> List(I, Rs1, Imm, T, ALUOp.Slt, BrOp.None, LSUOp.None, ALU),
    SLTIU  -> List(I, Rs1, Imm, T, ALUOp.Sltu, BrOp.None, LSUOp.None, ALU),
    XORI   -> List(I, Rs1, Imm, T, ALUOp.Xor, BrOp.None, LSUOp.None, ALU),
    ORI    -> List(I, Rs1, Imm, T, ALUOp.Or, BrOp.None, LSUOp.None, ALU),
    ANDI   -> List(I, Rs1, Imm, T, ALUOp.And, BrOp.None, LSUOp.None, ALU),
    SLLI   -> List(I, Rs1, Imm, T, ALUOp.Sll, BrOp.None, LSUOp.None, ALU),
    SRLI   -> List(I, Rs1, Imm, T, ALUOp.Srl, BrOp.None, LSUOp.None, ALU),
    SRAI   -> List(I, Rs1, Imm, T, ALUOp.Sra, BrOp.None, LSUOp.None, ALU),
    ADD    -> List(R, Rs1, Rs2, T, ALUOp.Add, BrOp.None, LSUOp.None, ALU),
    SUB    -> List(R, Rs1, Rs2, T, ALUOp.Sub, BrOp.None, LSUOp.None, ALU),
    SLL    -> List(R, Rs1, Rs2, T, ALUOp.Sll, BrOp.None, LSUOp.None, ALU),
    SLT    -> List(R, Rs1, Rs2, T, ALUOp.Slt, BrOp.None, LSUOp.None, ALU),
    SLTU   -> List(R, Rs1, Rs2, T, ALUOp.Sltu, BrOp.None, LSUOp.None, ALU),
    XOR    -> List(R, Rs1, Rs2, T, ALUOp.Xor, BrOp.None, LSUOp.None, ALU),
    SRL    -> List(R, Rs1, Rs2, T, ALUOp.Srl, BrOp.None, LSUOp.None, ALU),
    SRA    -> List(R, Rs1, Rs2, T, ALUOp.Sra, BrOp.None, LSUOp.None, ALU),
    OR     -> List(R, Rs1, Rs2, T, ALUOp.Or, BrOp.None, LSUOp.None, ALU),
    AND    -> List(R, Rs1, Rs2, T, ALUOp.And, BrOp.None, LSUOp.None, ALU),
    EBREAK -> List(I, Zero, Zero, F, ALUOp.Add, BrOp.None, LSUOp.None, EBreak),
    // TODO: fence and ecall
    // FENCE ->,
    // EALL ->,
  )
}

class DecodedBundle extends Bundle {
  val alu_oper1_type = UInt(OperType.WIDTH)
  val alu_oper2_type = UInt(OperType.WIDTH)

  val rs1   = UInt(5.W)
  val rs2   = UInt(5.W)
  val rd    = UInt(5.W)
  val rd_we = Bool()
  val imm   = UInt(32.W)

  val exec_type = UInt(ExecType.WIDTH)
  val alu_op    = UInt(ALUOp.WIDTH)
  val lsu_op    = UInt(LSUOp.WIDTH)
  val br_op     = UInt(BrOp.WIDTH)
}

class IDU extends Module {
  val io = IO(new Bundle {
    val inst    = Input(UInt(32.W))
    val decoded = new DecodedBundle
  })

  val inst = io.inst

  // Registers
  val rd  = inst(11, 7)
  val rs1 = inst(19, 15)
  val rs2 = inst(24, 20)

  // Immediates
  val immI = sign_extend(inst(31, 20), 32)
  val immS = sign_extend(inst(31, 25) ## inst(11, 7), 32)
  val immB = sign_extend(inst(31) ## inst(7) ## inst(30, 25) ## inst(11, 8) ## 0.U(1.W), 32)
  val immU = inst(31, 12) ## Fill(12, 0.U)
  val immJ = sign_extend(inst(31) ## inst(19, 12) ## inst(20) ## inst(30, 21) ## 0.U(1.W), 32)

  // Decode
  val fmt :: oper1_type :: oper2_type :: (we: Bool) :: alu_op :: br_op :: lsu_op :: exec_type :: Nil =
    ListLookup(inst, InstDecodeTable.default, InstDecodeTable.table)

  assert(fmt =/= InstFmt.Err, cf"Invalid instruction format. (Inst: ${inst})")

  // Choose immediate
  val imm = MuxLookup(fmt, 0.U)(
    Seq(
      InstFmt.I -> immI,
      InstFmt.S -> immS,
      InstFmt.B -> immB,
      InstFmt.U -> immU,
      InstFmt.J -> immJ
    )
  )

  // printf(cf"[IDU]: Inst: ${inst}, imm: ${imm}, rd: ${rd}, rs1: ${rs1}, rs2: ${rs2}, exec_type: ${exec_type}\n");

  // IO
  io.decoded.alu_oper1_type := oper1_type
  io.decoded.alu_oper2_type := oper2_type
  io.decoded.rs1            := rs1
  io.decoded.rs2            := rs2
  io.decoded.imm            := imm
  io.decoded.rd             := rd
  io.decoded.rd_we          := we
  io.decoded.alu_op         := alu_op
  io.decoded.lsu_op         := lsu_op
  io.decoded.br_op          := br_op
  io.decoded.exec_type      := exec_type
}
