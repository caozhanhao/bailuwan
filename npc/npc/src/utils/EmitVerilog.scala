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
  circt.stage.ChiselStage.emitSystemVerilogFile(new top.Top(), args, CommonEmitVerilogOptions.firtool)
}

object Emit4BitALUVerilog extends App {
  class ALU4BitParams extends top.CoreParams {
    override val XLEN:          Int = 4
    override val ALUShamtWidth: Int = 1
  }

  implicit val p: ALU4BitParams = new ALU4BitParams()
  circt.stage.ChiselStage.emitSystemVerilogFile(new core.ALU(), args, CommonEmitVerilogOptions.firtool)
}
