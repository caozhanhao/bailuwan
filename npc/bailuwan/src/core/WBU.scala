// Copyright (c) 2025-2026 caozhanhao
// SPDX-License-Identifier: MulanPSL-2.0

package core

import chisel3._
import chisel3.util._
import constants._
import utils.SignalProbe
import bailuwan.CoreParams

class WBU(
  implicit p: CoreParams)
    extends Module {
  val io = IO(new Bundle {
    val in = Flipped(Decoupled(new LSUOut))

    // RegFile
    val rd_addr = Output(UInt(5.W))
    val rd_data = Output(UInt(p.XLEN.W))
    val rd_we   = Output(Bool())

    // CSRFile and Exception
    val csr_rd_we   = Output(Bool())
    val csr_rd_addr = Output(UInt(12.W))
    val csr_rd_data = Output(UInt(p.XLEN.W))
    val csr_epc     = Output(UInt(p.XLEN.W))
    val mtvec       = Input(UInt(p.XLEN.W))
    val mepc        = Input(UInt(p.XLEN.W))

    val exception       = Output(new ExceptionInfo)
    val redirect_valid  = Output(Bool())
    val redirect_target = Output(UInt(p.XLEN.W))

    val icache_flush = Output(Bool())

    // Hazard
    val hazard = Output(new HazardInfo)
  })

  val pc          = io.in.bits.pc
  val inst        = io.in.bits.inst
  val excp        = io.in.bits.exception
  val is_trap_ret = io.in.bits.is_trap_return
  val is_fence_i  = io.in.bits.is_fence_i

  io.rd_addr := io.in.bits.rd_addr
  io.rd_data := io.in.bits.ls_out
  io.rd_we   := io.in.valid && io.in.bits.rd_we && !excp.valid

  // CSR
  io.csr_rd_we   := io.in.valid && io.in.bits.csr_rd_we && !excp.valid
  io.csr_rd_addr := io.in.bits.csr_rd_addr
  io.csr_rd_data := io.in.bits.csr_rd_data

  // Exception
  io.exception.valid := io.in.valid && excp.valid
  io.exception.cause := excp.cause
  io.exception.tval  := excp.tval
  io.csr_epc         := io.in.bits.pc

  // Redirect
  io.redirect_valid  := io.in.valid && (excp.valid || is_trap_ret || is_fence_i)
  io.redirect_target := MuxCase(
    io.mtvec,
    Seq(
      is_trap_ret -> io.mepc,
      is_fence_i  -> (io.in.bits.pc + 4.U)
    )
  )

  // FenceI
  io.icache_flush := io.in.valid && io.in.bits.is_fence_i

  // Hazard
  io.hazard.valid      := io.in.valid && io.in.bits.rd_we && !excp.valid
  io.hazard.rd         := io.in.bits.rd_addr
  io.hazard.data       := io.in.bits.ls_out
  io.hazard.data_valid := true.B

  io.in.ready := true.B

  if (p.Debug) {
    dontTouch(pc)
    dontTouch(inst)
  }

  // Exposed signals for difftest
  // Note that the width of a RegNext is not set based on the next or
  // init connections for Element types
  val difftest_pc = Reg(UInt(p.XLEN.W))
  difftest_pc := pc
  val difftest_inst = Reg(UInt(32.W))
  difftest_inst := inst
  SignalProbe(difftest_pc, "difftest_pc")
  SignalProbe(difftest_inst, "difftest_inst")
  SignalProbe(RegNext(io.in.fire), "difftest_ready")
}
