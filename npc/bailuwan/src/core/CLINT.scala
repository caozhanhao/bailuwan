// Copyright (c) 2025 caozhanhao
// SPDX-License-Identifier: MulanPSL-2.0

package core

import chisel3._
import chisel3.util._
import amba._

class CLINT(
  implicit val axi_prop: AXIProperty)
    extends Module {
  val io = IO(Flipped(new AXI4))

  // AXI4-Lite
  io.r.bits.last := true.B
  io.r.bits.id   := 0.U
  io.b.bits.id   := 0.U

  val mtime = RegInit(0.U(64.W))
  mtime := mtime + 1.U

  val addr_reg = RegInit(0.U(axi_prop.ADDR_WIDTH.W))
  addr_reg := Mux(io.ar.fire, io.ar.bits.addr, addr_reg)

  io.ar.ready    := true.B
  io.r.valid     := true.B
  io.r.bits.data := Mux(addr_reg === 0x02000000L.U, mtime(31, 0), mtime(63, 32))
  io.r.bits.resp := AXIResp.OKAY

  io.aw.ready    := true.B
  io.w.ready     := true.B
  io.b.valid     := true.B
  io.b.bits.resp := AXIResp.SLVERR
}
