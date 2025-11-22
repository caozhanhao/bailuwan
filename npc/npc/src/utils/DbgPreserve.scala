package utils

import chisel3._
import chisel3.util._
import top.CoreParams

class DbgPreserve(name: String, width: Int) extends BlackBox with HasBlackBoxInline {
  val io                   = IO(new Bundle {
    val data = Input(UInt())
  })
  override def desiredName = s"DbgPreserve_$name"

  setInline(
    s"DbgPreserve_$name" +
      s".sv",
    s"""
       |module DbgPreserve_$name(
       |    input [${width - 1}:0]data
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

object DbgPreserve {
  def apply(
    data:       UInt,
    name:       String
  )(
    implicit p: CoreParams
  ): Unit = {
    if (p.Debug) {
      val c = Module(new DbgPreserve(name, data.getWidth))
      c.io.data := data
    }
  }
}
