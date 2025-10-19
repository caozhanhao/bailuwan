package constants

import chisel3._
import chisel3.util._

object BrOp {
  val WIDTH = log2Ceil(9).W

  val Nop = 0.U(BrOp.WIDTH)
  val JAL  = 1.U(BrOp.WIDTH)
  val JALR = 2.U(BrOp.WIDTH)
  val BEQ  = 3.U(BrOp.WIDTH)
  val BNE  = 4.U(BrOp.WIDTH)
  val BLT  = 5.U(BrOp.WIDTH)
  val BGE  = 6.U(BrOp.WIDTH)
  val BLTU = 7.U(BrOp.WIDTH)
  val BGEU = 8.U(BrOp.WIDTH)
}
