// Copyright (c) 2025 caozhanhao
// SPDX-License-Identifier: MIT

package bailuwan

import chisel3._
import core._
import amba._

class Top extends Module {
  override val desiredName = "ysyx_25100251"

  implicit val p:        CoreParams  = CoreParams()
  implicit val axi_prop: AXIProperty = AXIProperty()

  val io = IO(new Bundle {
    val master    = new AXI4
    val slave     = Flipped(new AXI4)
    val interrupt = Input(Bool())
  })

  val core = Module(new Core)
  core.io <> io
}
