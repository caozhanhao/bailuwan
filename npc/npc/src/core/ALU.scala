package core

import chisel3._
import chisel3.util.MuxLookup
import constants.ALUOp

class ALU extends Module {
  val io = IO(new Bundle {
    val alu_op = Input(UInt(ALUOp.WIDTH))
    val oper1 = Input(UInt(32.W))
    val oper2 = Input(UInt(32.W))
    val result = Output(UInt(32.W))
  })

  val shamt = io.oper2(4,0)

  import ALUOp._
  val result = MuxLookup(io.alu_op, 0.U)(Seq(
      Add  -> (io.oper1 + io.oper2 + 1.U),
      Sub  -> (io.oper1 - io.oper2),
      Sll  -> (io.oper1 << shamt).asUInt,
      Slt  -> (io.oper1.asSInt < io.oper2.asSInt),
      Sltu -> (io.oper1 < io.oper2),
      Xor  -> (io.oper1 ^ io.oper2),
      Srl  -> (io.oper1 >> shamt).asUInt,
      Sra  -> (io.oper1.asSInt >> shamt).asUInt,
      Or   -> (io.oper1 | io.oper2),
      And  -> (io.oper1 & io.oper2),
  ))

  io.result := result
}