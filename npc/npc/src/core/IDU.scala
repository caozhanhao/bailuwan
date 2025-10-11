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
    ADD    -> List(R, Rs1, Rs2, T, ALUOp.Add, BrOp.None, LSUOp.None, ALU),
    ADDI   -> List(I, Rs1, Imm, T, ALUOp.Add, BrOp.None, LSUOp.None, ALU),
    LUI    -> List(U, Imm, Zero, T, ALUOp.Add, BrOp.None, LSUOp.None, ALU),
    LW     -> List(I, Rs1, Imm, T, ALUOp.Add, BrOp.None, LSUOp.LW, LSU),
    LBU    -> List(I, Rs1, Imm, T, ALUOp.Add, BrOp.None, LSUOp.LBU, LSU),
    SW     -> List(S, Rs1, Imm, F, ALUOp.Add, BrOp.None, LSUOp.SW, LSU),
    SB     -> List(S, Rs1, Imm, F, ALUOp.Add, BrOp.None, LSUOp.SB, LSU),
    JALR   -> List(I, PC, Four, T, ALUOp.Add, BrOp.JALR, LSUOp.None, ALU),
    EBREAK -> List(I, Zero, Zero, F, ALUOp.Add, BrOp.None, LSUOp.None, EBreak)
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
    val inst_valid = Input(Bool())
    val decoded = new DecodedBundle
  })

  val NOP = 0x00000013.U(32.W)
  val inst = Mux(io.inst_valid, io.inst, NOP)

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

  printf(cf"Decoded: (Inst: ${inst}, imm: ${imm}, rd: ${rd}, rs1: ${rs1}, rs2: ${rs2}, exec_type: ${exec_type}, alu_op: ${alu_op}, lsu_op: ${lsu_op}, br_op: ${br_op})")

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
