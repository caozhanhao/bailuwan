// Copyright (c) 2025 caozhanhao
// SPDX-License-Identifier: MIT

package bailuwan

case class CoreParams() {
  val XLEN:        Int     = 32
  val RegCount:    Int     = 16
  val ResetVector: Int     = 0x3000_0000 // FLASH
  val Debug:       Boolean = true
}
