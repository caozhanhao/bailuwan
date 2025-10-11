package utils

import chisel3._
import chisel3.util._

object Utils {
  def sign_extend(x: UInt, width: Int): UInt = {
    Fill(width - x.getWidth, x(x.getWidth - 1)) ## x
  }
  def zero_extend(x: UInt, width: Int): UInt = {
    Fill(width - x.getWidth, 0.U) ## x
  }
}