package constants

import chisel3._
import chisel3.util._

object LSUOp {
  val WIDTH = log2Ceil(9).W

  val None = 0.U(WIDTH)
  val LB   = 1.U(WIDTH)
  val LH   = 2.U(WIDTH)
  val LW   = 3.U(WIDTH)
  val LBU  = 4.U(WIDTH)
  val LHU  = 5.U(WIDTH)
  val SB   = 6.U(WIDTH)
  val SH   = 7.U(WIDTH)
  val SW   = 8.U(WIDTH)
}
