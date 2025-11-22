package utils

import chisel3._
import chisel3.util._
import top.CoreParams

class PerfCounter(name: String) extends BlackBox with HasBlackBoxInline {
  val io                   = IO(new Bundle {
    val clock = Input(Clock())
    val reset = Input(Reset())
    val cond  = Input(Bool())
  })
  override def desiredName = s"PerfCounter_$name"

  setInline(
    s"PerfCounter_$name.sv",
    s"""
       |module PerfCounter_$name(
       |    input clock,
       |    input reset,
       |    input cond
       |);
       |
       |`ifdef VERILATOR
       |  reg [63:0] $name /* verilator public_flat_rd */;
       |
       |  always @(posedge clock) begin
       |    if (reset)
       |      $name <= 64'd0;
       |    else if (cond)
       |      $name <= $name + 64'd1;
       |  end
       |`endif
       |
       |endmodule
       |""".stripMargin
  )
}

object PerfCounter {
  def apply(
    cond:       Bool,
    name:       String
  )(
    implicit p: CoreParams
  ): Unit = {
    if (p.Debug) {
      val c = Module(new PerfCounter(name))
      c.io.clock := Module.clock
      c.io.reset := Module.reset
      c.io.cond  := cond
    }
  }
}
