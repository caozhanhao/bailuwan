// Copyright (c) 2025 caozhanhao
// SPDX-License-Identifier: MulanPSL-2.0

package deprecated

import amba._
import chisel3._

class SimConsoleOutput(
  implicit val axi_prop: AXIProperty)
    extends Module {
  val io = IO(Flipped(new AXI4))

  // AXI4-Lite
  io.r.bits.last := true.B
  io.r.bits.id   := 0.U
  io.b.bits.id   := 0.U

  io.ar.ready    := false.B
  io.r.valid     := false.B
  io.r.bits.data := 0.U
  io.r.bits.resp := AXIResp.SLVERR

  val write_enable = io.aw.valid && io.w.valid
  val char         = io.w.bits.data(7, 0)

  io.aw.ready := write_enable
  io.w.ready  := write_enable

  io.b.valid     := RegNext(write_enable, false.B)
  io.b.bits.resp := AXIResp.OKAY

  when(write_enable) {
    printf("%c", char)
  }
}
