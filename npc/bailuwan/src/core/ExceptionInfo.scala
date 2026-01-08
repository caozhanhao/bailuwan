// Copyright (c) 2025-2026 caozhanhao
// SPDX-License-Identifier: MulanPSL-2.0

package core

import chisel3._
import bailuwan.CoreParams

class ExceptionInfo(
  implicit p: CoreParams)
    extends Bundle {
  val valid = Bool()
  val cause = UInt(p.XLEN.W)
  val tval  = UInt(p.XLEN.W)
}