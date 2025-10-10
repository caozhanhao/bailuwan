// See README.md for license details.

package top

import org.scalatest.freespec.AnyFreeSpec
import org.scalatest.matchers.must.Matchers

import chisel3._
import chisel3.experimental.BundleLiterals._
import chisel3.simulator.EphemeralSimulator._
import org.scalatest.freespec.AnyFreeSpec
import org.scalatest.matchers.must.Matchers

class RV32EEmu(program: Seq[Byte]) {
  // TODO
}

class NPCTestTop extends Module {
  val npc = Module(new Top)
  npc.io.mem.inst := 0.U
}

class NPCSpec extends AnyFreeSpec with Matchers {
  "NPC should be the same as RV32EEmu" in {
    simulate(new NPCTestTop) { dut =>
      println("TODO")
    }
  }
}