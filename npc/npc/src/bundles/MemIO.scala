package bundles

import chisel3._

class MemIO extends Bundle {
  val addr         = Input(UInt(32.W))
  val read_enable  = Input(Bool())
  val write_enable = Input(Bool())
  val write_mask   = Input(UInt(8.W))
  val write_data   = Input(UInt(32.W))
  val data_out     = Output(UInt(32.W))
}