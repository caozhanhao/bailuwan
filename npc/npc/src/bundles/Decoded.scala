package bundles

import chisel3._
import constants.ALUOp

class DecodedBundle extends Bundle {
  val oper1  = UInt(32.W)
  val oper2  = UInt(32.W)
  val dest   = UInt(5.W)
  val alu_op = UInt(ALUOp.WIDTH)
  val imm    = UInt(32.W)
}
