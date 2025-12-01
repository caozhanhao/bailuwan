// Copyright (c) 2025 caozhanhao
// SPDX-License-Identifier: MIT

package bailuwan

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

class BaiLuWanTestTop extends Module {
}

class BaiLuWanTestSpec extends AnyFreeSpec with Matchers {
  "BaiLuWan should be the same as RV32EEmu" in {
    simulate(new BaiLuWanTestTop) { dut =>
      println("TODO")
    }
  }
}
