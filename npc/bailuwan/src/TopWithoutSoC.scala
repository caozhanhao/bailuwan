// Copyright (c) 2025 caozhanhao
// SPDX-License-Identifier: MIT

package bailuwan

import amba._
import chisel3._
import utils.DPICMem

class TopWithoutSoC(
  implicit p: CoreParams,
  axi_prop:   AXIProperty)
    extends Module {
  override val desiredName = "TopWithoutSoC"

  val core = Module(new Core)
  core.io.interrupt := false.B

  core.io.slave.aw.valid := false.B
  core.io.slave.aw.bits  := 0.U.asTypeOf(core.io.slave.aw.bits)
  core.io.slave.w.valid  := false.B
  core.io.slave.w.bits   := 0.U.asTypeOf(core.io.slave.w.bits)
  core.io.slave.b.ready  := false.B
  core.io.slave.ar.valid := false.B
  core.io.slave.ar.bits  := 0.U.asTypeOf(core.io.slave.ar.bits)
  core.io.slave.r.ready  := false.B

  val mem = Module(new DPICMem)
  core.io.master <> mem.io
}
