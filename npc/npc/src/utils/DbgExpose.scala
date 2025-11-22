package utils

import chisel3._
import chisel3.util._
import top.CoreParams

class DbgExpose(name: String) extends BlackBox with HasBlackBoxInline {
  val io                   = IO(new Bundle {
    val data  = Input(UInt())
  })
  override def desiredName = s"DbgExpose_$name"

  setInline(
    s"DbgExpose_$name.sv",
    s"""
       |module DbgExpose_$name(
       |    input data
       |);
       |
       |`ifdef VERILATOR
       |  wire [63:0] $name /* verilator public_flat_rd */;
       |  assign $name = val;
       |`endif
       |
       |endmodule
       |""".stripMargin
  )
}

object DbgExpose {
  def apply(
    data:       UInt,
    name:       String
  )(
    implicit p: CoreParams
  ): Unit = {
    if (p.Debug) {
      val c = Module(new DbgExpose(name))
      c.io.data := data
    }
  }
}
