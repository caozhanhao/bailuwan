package bundles

import chisel3._
import chisel3.util._
import constants._

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
