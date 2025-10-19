package core

import chisel3._
import chisel3.util.HasBlackBoxInline

class EBreak extends HasBlackBoxInline {
  val io = IO(new Bundle {
    val en = Input(Bool())
  })
  setInline(
    "EBreak.sv",
    """
      |module EBreak(
      |  input  en
      |);
      |  import "DPI-C" function void ebreak_handler();
      |  always @(*) begin
      |    if (en)
      |      ebreak_handler();
      |  end
      |endmodule
      |""".stripMargin
  )
}
