// Copyright (c) 2025 caozhanhao
// SPDX-License-Identifier: MIT

package constants

import chisel3._
import chisel3.util._
import bailuwan.CoreParams

object ExceptionCode {
  val InstructionAddressMisaligned = 0.U
  val InstructionAccessFault = 1.U
  val IllegalInstruction = 2.U
  val Breakpoint = 3.U
  val LoadAddressMisaligned = 4.U
  val LoadAccessFault = 5.U
  val StoreAMOAddressMisaligned = 6.U
  val StoreAMOAccessFault = 7.U
  val EnvironmentCallFromUMode = 8.U
  val EnvironmentCallFromSMode = 9.U
  val EnvironmentCallFromMMode = 11.U
  val InstructionPageFault = 12.U
  val LoadPageFault = 13.U
  val StoreAMOPageFault = 15.U
}


