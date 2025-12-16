// Copyright (c) 2025 caozhanhao
// SPDX-License-Identifier: MulanPSL-2.0

package bailuwan

import chisel3._
import core._
import amba._

class Top(
  implicit p: CoreParams,
  axi_prop:   AXIProperty)
    extends Module {
  override val desiredName = "ysyx_25100251"

  val io = IO(new Bundle {
    val master    = new AXI4
    val slave     = Flipped(new AXI4)
    val interrupt = Input(Bool())
  })

  val core = Module(new Core)
  core.io <> io
}
