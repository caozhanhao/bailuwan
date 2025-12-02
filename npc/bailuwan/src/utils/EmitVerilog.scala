// Copyright (c) 2025 caozhanhao
// SPDX-License-Identifier: MIT

package utils

import bailuwan.CoreParams
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
  val builder    = OParser.builder[CoreParams]
  val arg_parser = {
    import builder._
    OParser.sequence(
      programName("bailuwan-generator"),
      head("bailuwan", "0.0.1"),
      opt[String]("reset-vector")
        .action((x, c) => {
          val v = if (x.startsWith("0x")) Integer.parseUnsignedInt(x.drop(2), 16) else x.toInt
          c.copy(ResetVector = v)
        })
        .text("set the reset vector"),
      opt[Boolean]("debug")
        .action((x, c) => c.copy(Debug = x))
        .text("enable debug mode")
    )
  }

  val separatorIndex         = args.indexOf("--")
  val (blwArgs, firtoolArgs) = if (separatorIndex == -1) {
    (args, Array.empty[String])
  } else {
    (args.take(separatorIndex), args.drop(separatorIndex + 1))
  }

  OParser.parse(arg_parser, blwArgs, CoreParams()) match {
    case Some(config) =>
      println(
        s"[Info] Params: ResetVector=0x${config.ResetVector.toHexString}, XLEN=${config.XLEN}, Debug=${config.Debug}"
      )

      if(firtoolArgs.nonEmpty) {
        println(s"[Info] Passing extra args to Firtool: ${firtoolArgs.mkString(" ")}")
      }

      implicit val p:        CoreParams  = config
      implicit val axi_prop: AXIProperty = AXIProperty()

      circt.stage.ChiselStage.emitSystemVerilogFile(new bailuwan.Top(), firtoolArgs, CommonEmitVerilogOptions.firtool)

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
