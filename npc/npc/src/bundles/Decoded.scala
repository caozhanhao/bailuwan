package bundles

import chisel3._
import chisel3.util._
import constants._

object OperType {
  val WIDTH = log2Ceil(6).W

  val Rs1 = 0.U(OperType.WIDTH)
  val Rs2 = 1.U(OperType.WIDTH)
  val Imm  = 2.U(OperType.WIDTH)
  val Zero = 3.U(OperType.WIDTH)
  val Four = 4.U(OperType.WIDTH)
  val PC   = 5.U(OperType.WIDTH)
}

object ExecType {
  val WIDTH = log2Ceil(2).W

  val ALU = 0.U(ExecType.WIDTH)
  val LSU = 1.U(ExecType.WIDTH)
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
