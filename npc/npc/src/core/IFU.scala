package core

import chisel3._

class IFU extends Module {
  val io = IO(new Bundle {
    val pc = Input(UInt(32.W))
    val inst = Output(UInt(32.W))
  })
}