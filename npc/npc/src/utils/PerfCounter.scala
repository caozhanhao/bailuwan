package utils

import chisel3._
import chisel3.util._

class PerfCounter extends Module {
  val io = IO(new Bundle {
    val cond = Input(Bool())
    val out = Output(UInt(64.W))
  })

  val reg = RegInit(0.U(64.W))
  reg := Mux(io.cond, reg + 1.U, reg)
  io.out := reg
}

object PerfCounter {
  def apply(cond: Bool): UInt = {
    val perf = Module(new PerfCounter)
    perf.io.cond := cond
    perf.io.out
  }
}
