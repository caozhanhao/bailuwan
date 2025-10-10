package bundles

import chisel3._
import chisel3.util._
import constants.ALUOp

object OperType {
  val WIDTH = log2Ceil(4).W

  val Reg  = 0.U(OperType.WIDTH)
  val Imm  = 1.U(OperType.WIDTH)
  val Zero = 2.U(OperType.WIDTH)
  val PC   = 3.U(OperType.WIDTH)
}

object BrType {
  val WIDTH = log2Ceil(9).W

  val None = 0.U(BrType.WIDTH)
  val JAL  = 1.U(BrType.WIDTH)
  val JALR = 2.U(BrType.WIDTH)
  val BEQ  = 3.U(BrType.WIDTH)
  val BNE  = 4.U(BrType.WIDTH)
  val BLT  = 5.U(BrType.WIDTH)
  val BGE  = 6.U(BrType.WIDTH)
  val BLTU = 7.U(BrType.WIDTH)
  val BGEU = 8.U(BrType.WIDTH)
}

class DecodedBundle extends Bundle {
  val alu_oper1_type = UInt(OperType.WIDTH)
  val alu_oper2_type = UInt(OperType.WIDTH)

  val br_type = UInt(BrType.WIDTH)

  val rs1 = UInt(5.W)
  val rs2 = UInt(5.W)
  val rd  = UInt(5.W)
  val rd_we  = Bool()
  val imm = UInt(32.W)

  val alu_op = UInt(ALUOp.WIDTH)
}
