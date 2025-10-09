// See README.md for license details.

package top

import chisel3._
import chisel3.experimental.BundleLiterals._
import chisel3.simulator.EphemeralSimulator._
import org.scalatest.freespec.AnyFreeSpec
import org.scalatest.matchers.must.Matchers

import scala.collection.mutable

class RV32EEmu(program: Seq[Byte]) {
  // TODO
}

class NPCSpec extends AnyFreeSpec with Matchers {
  "NPC should be the same as RV32EEmu" in {
    println("TODO")
  }
}