package bundles

import chisel3._
import chisel3.util._
import constants._

class WriteBackIn extends Bundle {
  // Register File
  val src_type = UInt(ExecType.WIDTH)
  val alu_out  = UInt(32.W)
  val lsu_out  = UInt(32.W)

  // PC
  val snpc      = UInt(32.W)
  val br_taken  = Bool()
  val br_target = UInt(32.W)
}

class WriteBackOut extends Bundle {
  // Register File
  val rd_data = UInt(32.W)

  // PC
  val dnpc = UInt(32.W)
}
