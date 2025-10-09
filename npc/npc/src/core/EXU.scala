package core

import chisel3._

import bundles.DecodedBundle

class EXU extends Module {
  val io = IO(new Bundle {
    val decoded = Input(new DecodedBundle)
  })

  val alu = Module(new ALU)

  alu.io.oper1 := io.decoded.oper1
  alu.io.oper2 := io.decoded.oper2
  alu.io.alu_op := io.decoded.alu_op
}