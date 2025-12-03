// Copyright (c) 2025 caozhanhao
// SPDX-License-Identifier: MIT

package core

import chisel3._
import chisel3.util.HasBlackBoxInline

class EBreak extends HasBlackBoxInline {
  val io = IO(new Bundle {
    val clock = Input(Clock())
    val en = Input(Bool())
  })
  setInline(
    "EBreak.sv",
    """
      |module EBreak(
      |  input clock,
      |  input en
      |);
      |  import "DPI-C" function void ebreak_handler();
      |  always @(posedge clock) begin
      |    if (en)
      |      ebreak_handler();
      |  end
      |endmodule
      |""".stripMargin
  )
}

//class TempEBreakForSTA extends Module {
//  val io = IO(new Bundle {
//    val en = Input(Bool())
//  })
//}