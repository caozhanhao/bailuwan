// Copyright (c) 2025 caozhanhao
// SPDX-License-Identifier: MulanPSL-2.0

package bailuwan

case class CoreParams(
  XLEN:        Int = 32,
  RegCount:    Int = 16,
  ResetVector: Int = 0x3000_0000, // FLASH
  Debug: Boolean = true) {}
