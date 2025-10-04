package top

import chisel3._
import chisel3.util.{MuxCase, MuxLookup}
import chisel3.util.experimental.BoringUtils

import scala.collection.mutable

class sCPU(program: Seq[Byte]) extends Module {
  val io = IO(new Bundle {
    val out      = Output(UInt(8.W))
    val outValid = Output(Bool())
  })

  val rom  = VecInit(program.toIndexedSeq.map(_.S(32.W).asUInt))
  val regs = RegInit(VecInit(Seq.fill(4)(0.U(8.W))))
  val pc   = RegInit(0.U(8.W))

  val output      = RegInit(0.U(8.W))
  val outputValid = RegInit(false.B)
  io.out      := output
  io.outValid := outputValid

  // decode
  val inst = rom(pc)
  val op   = inst(7, 6)
  val rd   = inst(5, 4)
  val rs1  = inst(3, 2)
  val rs2  = inst(1, 0)

  import sCPU._
  regs(rd) := MuxCase(regs(rd), Seq(
    (op === LI.U) -> inst(3, 0),
    (op === ADD.U) -> (regs(rs1) + regs(rs2))
  ))

  output := Mux(op === OUT.U, regs(rs1), output)
  outputValid := Mux(op === OUT.U, true.B, outputValid)

  val brTaken = op === BNER0.U && regs(0) =/= regs(rs2)
  pc := Mux(brTaken, inst(5, 2), pc + 1.U)
}

object sCPU {
  val ADD   = 0
  val LI    = 2
  val BNER0 = 3
  val OUT   = 1

  //  7  6 5  4 3   2 1   0
  // +----+----+-----+-----+
  // | 00 | rd | rs1 | rs2 | R[rd]=R[rs1]+R[rs2]                add
  // +----+----+-----+-----+
  // | 10 | rd |    imm    | R[rd]=imm                          li
  // +----+----+-----+-----+
  // | 11 |   addr   | rs2 | if (R[0]!=R[rs2]) PC=addr          bner0
  // +----+----------+-----+
  // | 01 |    | rs1 |     | output rs1                         out
  // +----+----------+-----+
  def assemble(code: String): Seq[Byte] = {
    val lines  = code.split("\n").filter(_.nonEmpty)
    val labels = mutable.Map[String, Int]()
    val insts  = mutable.Buffer[String]()
    var addr   = 0
    for (raw_line <- lines) {
      val line = raw_line.split("#")(0)
      if (line.contains(":"))
        labels += (line.split(":")(0) -> addr)
      else {
        insts += line
        addr += 1
      }
    }

    def operand(name: String, shift: Int = 0) = {
      if (name.startsWith("r"))
        name.substring(1).toInt << shift
      else
        throw new Exception(s"Invalid operand: $name")
    }

    def label(name: String, shift: Int = 0) = {
      if (!labels.contains(name))
        throw new Exception(s"Label $name not found")
      labels(name) << shift
    }

    insts.map { inst =>
      {
        inst.trim.split("[ ,]+") match {
          case Array("add", rd, rs1, rs2) =>
            (ADD << 6) | operand(rd, 4) | operand(rs1, 2) | operand(rs2)
          case Array("li", rd, imm)       =>
            (LI << 6) | operand(rd, 4) | imm.toInt
          case Array("bner0", rs2, addr)  =>
            (BNER0 << 6) | label(addr, 2) | operand(rs2)
          case Array("out", rs1)          =>
            (OUT << 6) | operand(rs1, 2)
        }
      }.toByte
    }.toSeq
  }
}
