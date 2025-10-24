package utils

import chisel3._

class LFSR5 extends Module {
  val io = IO(new Bundle {
    val out = Output(UInt(32.W))
  })

  val lfsr = RegInit("b11111".U(5.W))

  lfsr := (0.U ^ lfsr(0)) ## lfsr(4) ## (lfsr(3) ^ lfsr(0)) ## lfsr(2) ## lfsr(1)

  io.out := lfsr
}

object RandomDelay {
  def apply(signal: Bool): Bool = {
    val lfsr    = Module(new LFSR5())
    val counter = RegInit(0.U(5.W))
    val pending = RegInit(false.B)
    val out     = RegInit(false.B)

    val prev  = RegNext(signal, false.B)
    val start = signal && !prev

    out := false.B

    when(start && !pending) {
      pending := true.B
      counter := lfsr.io.out
    }.elsewhen(pending) {
      when(counter =/= 0.U) {
        counter := counter - 1.U
      }.otherwise {
        out     := true.B
        pending := false.B
      }
    }

    out
  }
}