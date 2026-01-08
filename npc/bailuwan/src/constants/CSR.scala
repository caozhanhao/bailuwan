// Copyright (c) 2025-2026 caozhanhao
// SPDX-License-Identifier: MulanPSL-2.0

package constants

import chisel3._
import chisel3.util._

object CSR {
  val mstatus   = 0x300.U(12.W)
  val mtvec     = 0x305.U(12.W)
  val mepc      = 0x341.U(12.W)
  val mcause    = 0x342.U(12.W)
  val mtval     = 0x343.U(12.W)
  val mcycle    = 0xb00.U(12.W)
  val mcycleh   = 0xb80.U(12.W)
  val mvendorid = 0xf11.U(12.W)
  val marchid   = 0xf12.U(12.W)
}

object CSROp {
  val WIDTH = log2Ceil(4).W

  val Nop = 0.U(WIDTH)
  val RW  = 1.U(WIDTH)
  val RS  = 2.U(WIDTH)
  val RC  = 3.U(WIDTH)
}
