package constants

import chisel3._
import chisel3.util._

object ALUOp {
  val WIDTH = log2Ceil(10).W

  val Add  = 0.U(WIDTH)
  val Sub  = 1.U(WIDTH)
  val Sll  = 2.U(WIDTH)
  val Slt  = 3.U(WIDTH)
  val Sltu = 4.U(WIDTH)
  val Xor  = 5.U(WIDTH)
  val Srl  = 6.U(WIDTH)
  val Sra  = 7.U(WIDTH)
  val Or   = 8.U(WIDTH)
  val And  = 9.U(WIDTH)
}
