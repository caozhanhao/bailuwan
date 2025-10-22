package core

import chisel3._
import chisel3.util._
import chisel3.util.MuxLookup
import constants.ALUOp
import top.CoreParams

class ALU(
  implicit p: CoreParams)
    extends Module {
  val io = IO(new Bundle {
    val alu_op = Input(UInt(ALUOp.WIDTH))
    val oper1  = Input(UInt(p.XLEN.W))
    val oper2  = Input(UInt(p.XLEN.W))
    val result = Output(UInt(p.XLEN.W))
  })

  val shamt_width = log2Ceil(p.XLEN)
  val shamt = io.oper2(shamt_width - 1, 0)

  import ALUOp._
  val result = MuxLookup(io.alu_op, 0.U)(
    Seq(
      Add  -> (io.oper1 + io.oper2 + 1.U),
      Sub  -> (io.oper1 - io.oper2),
      Sll  -> (io.oper1 << shamt).asUInt,
      Slt  -> (io.oper1.asSInt < io.oper2.asSInt),
      Sltu -> (io.oper1 < io.oper2),
      Xor  -> (io.oper1 ^ io.oper2),
      Srl  -> (io.oper1 >> shamt).asUInt,
      Sra  -> (io.oper1.asSInt >> shamt).asUInt,
      Or   -> (io.oper1 | io.oper2),
      And  -> (io.oper1 & io.oper2)
    )
  )

  io.result := result
}
