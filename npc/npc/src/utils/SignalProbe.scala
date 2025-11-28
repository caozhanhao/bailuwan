package utils

import chisel3._
import chisel3.util._
import top.CoreParams

class SignalProbe(name: String, width: Int) extends BlackBox with HasBlackBoxInline {
  val io                   = IO(new Bundle {
    val data = Input(UInt(width.W))
  })
  override def desiredName = s"SignalProbe_$name"

  val wire_name = s"exposed_signal_$name"
  setInline(
    s"SignalProbe_$name" +
      s".sv",
    s"""
       |module SignalProbe_$name(
       |    input [${width - 1}:0] data
       |);
       |
       |`ifdef VERILATOR
       |  wire [${width - 1}:0] $wire_name /* verilator public_flat_rd */;
       |  assign $wire_name = data;
       |`endif
       |
       |endmodule
       |""".stripMargin
  )
}

object SignalProbe {
  def apply[T <: Data](
    data:       T,
    name:       String
  )(
    implicit p: CoreParams
  ): Unit = {
    if (p.Debug) {
      val c = Module(new SignalProbe(name, data.getWidth))
      c.io.data := data.asUInt
    }
  }
}
