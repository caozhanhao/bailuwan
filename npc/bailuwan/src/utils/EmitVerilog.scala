// Copyright (c) 2025 caozhanhao
// SPDX-License-Identifier: MIT

package utils

object CommonEmitVerilogOptions {
  val firtool = Array(
    "--lowering-options=" + List(
      // make yosys happy
      // see https://github.com/llvm/circt/blob/main/docs/VerilogGeneration.md
      "disallowLocalVariables",
      "disallowPackedArrays",
      "locationInfoStyle=wrapInAtSquareBracket"
    ).reduce(_ + "," + _)
  )
}

object EmitVerilog extends App {
  circt.stage.ChiselStage.emitSystemVerilogFile(new bailuwan.Top(), args, CommonEmitVerilogOptions.firtool)
}

object Emit4BitALUVerilog extends App {
  class ALU4BitParams extends bailuwan.CoreParams {
    override val XLEN: Int = 4
  }

  implicit val p: ALU4BitParams = new ALU4BitParams()
  circt.stage.ChiselStage.emitSystemVerilogFile(new core.ALU(), args, CommonEmitVerilogOptions.firtool)
}
