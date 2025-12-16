// Copyright (c) 2025 caozhanhao
// SPDX-License-Identifier: MulanPSL-2.0

package utils

import bailuwan._
import amba.AXIProperty
import scopt.OParser

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
  case class CmdOptions(reset_vector: Int = 0x3000_0000, without_soc: Boolean = false) {}

  val builder    = OParser.builder[CmdOptions]
  val arg_parser = {
    import builder._
    OParser.sequence(
      programName("bailuwan-generator"),
      head("bailuwan", "0.0.1"),
      opt[String]("reset-vector")
        .action((x, c) => {
          val v = if (x.startsWith("0x")) Integer.parseUnsignedInt(x.drop(2), 16) else x.toInt
          c.copy(reset_vector = v)
        })
        .text("set the reset vector"),
      opt[Boolean]("without-soc")
        .action((x, c) => c.copy(without_soc = x))
        .text("run without ysyxSoC")
    )
  }

  val separatorIndex         = args.indexOf("--")
  val (blwArgs, firtoolArgs) = if (separatorIndex == -1) {
    (args, Array.empty[String])
  } else {
    (args.take(separatorIndex), args.drop(separatorIndex + 1))
  }

  OParser.parse(arg_parser, blwArgs, CmdOptions()) match {
    case Some(config) =>
      println(
        s"[Info] Params: ResetVector=0x${config.reset_vector.toHexString}, ysyxSoc=${!config.without_soc}"
      )

      if (firtoolArgs.nonEmpty) {
        println(s"[Info] Passing extra args to Firtool: ${firtoolArgs.mkString(" ")}")
      }

      implicit val p:        CoreParams  = CoreParams(ResetVector = config.reset_vector)
      implicit val axi_prop: AXIProperty = AXIProperty()

      circt.stage.ChiselStage.emitSystemVerilogFile(
        if (config.without_soc) { new TopWithoutSoC() }
        else { new Top() },
        firtoolArgs,
        CommonEmitVerilogOptions.firtool
      )

    case _ =>
      System.exit(1)
  }
}

object Emit4BitALUVerilog extends App {
  class ALU4BitParams extends bailuwan.CoreParams {
    override val XLEN: Int = 4
  }

  implicit val p: ALU4BitParams = new ALU4BitParams()
  circt.stage.ChiselStage.emitSystemVerilogFile(new core.ALU(), args, CommonEmitVerilogOptions.firtool)
}
