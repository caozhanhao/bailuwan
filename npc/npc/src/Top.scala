package top

import chisel3._

class TopIO extends Bundle {
  val in  = Input(new InBundle)
  val out = Output(new OutBundle)
}

class Top extends Module {
  val io = IO(new TopIO)
}
