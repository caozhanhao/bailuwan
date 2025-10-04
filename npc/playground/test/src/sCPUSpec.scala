// See README.md for license details.

package top

import chisel3._
import chisel3.experimental.BundleLiterals._
import chisel3.simulator.EphemeralSimulator._
import org.scalatest.freespec.AnyFreeSpec
import org.scalatest.matchers.must.Matchers

import scala.collection.mutable

class sCPUEmu(program: Seq[Byte]) {
  var pc       = 0
  var out      = 0
  var outValid = false
  val regs     = Array.fill(4)(0)

  def step(): Unit = {
    val op     = program(pc) & 0xff
    val opcode = (op >> 6) & 0x03

    opcode match {
      case 0 => // add: 00 rd rs1 rs2
        val rd  = (op >> 4) & 0x03
        val rs1 = (op >> 2) & 0x03
        val rs2 = op & 0x03
        regs(rd) = (regs(rs1) + regs(rs2)) & 0xff
        pc += 1

      case 1 => // out: 01 -- rs1 --
        val rs1 = (op >> 2) & 0x03
        out = regs(rs1) & 0xff
        outValid = true
        pc += 1

      case 2 => // li: 10 rd imm(4)
        val rd  = (op >> 4) & 0x03
        val imm = op & 0x0f // 4-bit immediate, zero-extended
        regs(rd) = imm & 0xff
        pc += 1

      case 3 => // bner0: 11 addr(4) rs2
        val addr = (op >> 2) & 0x0f
        val rs2  = op & 0x03
        if (regs(0) != regs(rs2))
          pc = addr
        else
          pc += 1
    }
  }
}

class sCPUSpec extends AnyFreeSpec with Matchers {
  "sCPU should calculate the sum" in {
    val code =
      """
        |li r0, 10        # load imm 10 for loop bound
        |li r1, 0         # i = 0
        |li r2, 0         # sum = 0
        |li r3, 1         # load step imm 1 (step -> r3)
        |
        |loop:
        |add r1, r1, r3   # i += 1
        |add r2, r2, r1   # sum += i
        |bner0 r1, loop   # branch to %4 if i != 10
        |out r2           # set output as sum
        |
        |halt:
        |bner0 r3, halt   # b 2 != 11, %7 (dead loop)
        |""".stripMargin

    val program = sCPU.assemble(code)

    println("Program:")
    for ((code, i) <- program.zipWithIndex)
      println(s"0x${i.toHexString}: 0x${(code & 0xff).toBinaryString.reverse.padTo(8, '0').reverse}")

    // For PA1 Report
    val pa1 = new GenPA1ReportStateMachine(program)
    pa1.runAndDumpMermaid(50)

    simulate(new sCPUTestTop(program)) { dut =>
      dut.reset.poke(true.B)
      dut.clock.step()
      dut.reset.poke(false.B)

      val sim = new sCPUEmu(program)

      val cycles = 50
      for (i <- 0 until cycles) {
        dut.clock.step()

        sim.step()

        assert(dut.io.pc.peek().litValue == sim.pc)
        assert((dut.io.outValid.peek().litValue == 1) == sim.outValid)

        for (i <- 0 until 4)
          assert(dut.io.regs(i).peek().litValue == sim.regs(i))

        if (sim.outValid)
          assert(dut.io.out.peek().litValue == sim.out)

        println(s"Cycle $i:")
        println(s"  pc: ${dut.io.pc.peek().litValue}")
        if (dut.io.outValid.peek().litToBoolean)
          println(s"  out: ${dut.io.out.peek().litValue}")
        else
          println(s"  out: <none>")
      }
    }
  }
}

// PA1 报告中的必答题: 画出计算 `1+2+...+100` 的程序的状态机
class GenPA1ReportStateMachine(program: Seq[Byte]) extends sCPUEmu(program) {
  // 1 (4-bit) PC + 4 (8-bit) GPR = 36 bits

  import collection.mutable

  val state_id_map = mutable.Map[Long, Int](
    // Initial state
    (0x00000000L -> 0)
  )
  val state_map    = mutable.Map[Int, Long](
    // Initial state
    (0 -> 0)
  )

  var next_state_id = 1

  var current_state = 0

  // Current State ID -> Next State ID
  var transfer_map = mutable.Map[Int, Int]()

  override def step(): Unit = {
    super.step()
    val next_state = (pc.toLong << 32) | regs.zipWithIndex.map { case (reg, i) => (reg.toLong << (i * 8)) }.sum
    if (!state_id_map.contains(next_state)) {
      state_id_map(next_state) = next_state_id
      state_map(next_state_id) = next_state
      next_state_id += 1
    }

    val id = state_id_map(next_state)
    transfer_map(current_state) = id

    // println(s"Transfer $current_state -> ${id}")

    current_state = id
  }

  private def unpackState(state: Long): (Int, Array[Int]) = {
    val pc   = ((state >>> 32) & 0xffffffffL).toInt
    val regs = Array.tabulate(4) { i =>
      ((state >>> (i * 8)) & 0xffL).toInt
    }
    (pc, regs)
  }

  def dumpMermaid(): Unit = {
    println("```mermaid")
    println("graph TD")
    // nodes (sorted by id for determinism)
    for ((id, state) <- state_map.toSeq.sortBy(_._1)) {
      val (p, r) = unpackState(state)
      val label  = s"PC=${p} R0=${r(0)} R1=${r(1)} R2=${r(2)} R3=${r(3)}"
      println(s"""  S$id["$label"]""")
    }

    // edges
    for ((from, to) <- transfer_map.toSeq.sortBy(_._1)) {
      println(s"  S$from --> S$to")
    }
    println("```")
  }

  def runAndDumpMermaid(cycles: Int): Unit = {
    for (i <- 0 until cycles) {
      step()
    }
    dumpMermaid()
  }
}
