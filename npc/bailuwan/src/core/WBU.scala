// Copyright (c) 2025 caozhanhao
// SPDX-License-Identifier: MIT

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

    val exception       = Output(new ExceptionInfo)
    val redirect_valid  = Output(Bool())
    val redirect_target = Output(UInt(p.XLEN.W))
  })

  val exu_out = io.in.bits.from_exu
  val lsu_out = io.in.bits.read_data
  val excp    = io.in.bits.exception

  import ExecType._
  val rd_data = MuxLookup(exu_out.src_type, 0.U)(
    Seq(
      ALU -> exu_out.alu_out,
      LSU -> lsu_out,
      CSR -> exu_out.csr_out
    )
  )

  io.rd_addr := exu_out.rd_addr
  io.rd_data := rd_data
  io.rd_we   := io.in.valid && exu_out.rd_we && !excp.valid

  // CSR
  val is_csr = exu_out.src_type === ExecType.CSR
  io.csr_rd_we   := io.in.valid && !excp.valid && is_csr
  io.csr_rd_addr := exu_out.csr_rd_addr
  io.csr_rd_data := exu_out.csr_rd_data

  // Exception
  io.exception.valid := io.in.valid && excp.valid
  io.exception.cause := excp.cause
  io.exception.tval  := excp.tval
  io.csr_epc         := io.in.bits.pc

  // Redirect
  io.redirect_valid  := io.in.valid && (excp.valid || exu_out.is_trap_return)
  io.redirect_target := exu_out.csr_out

  io.in.ready := true.B

  if (p.Debug) {
    dontTouch(io.in.bits.pc)
    dontTouch(io.in.bits.inst)
  }

  // Exposed signals for difftest
  // Note that the width of a RegNext is not set based on the next or
  // init connections for Element types
  val wbu_pc = Reg(UInt(p.XLEN.W))
  wbu_pc := io.in.bits.pc
  val wbu_inst = Reg(UInt(32.W))
  wbu_inst := io.in.bits.inst
  SignalProbe(wbu_pc, "wbu_pc")
  SignalProbe(wbu_inst, "wbu_inst")
  SignalProbe(RegNext(io.in.fire), "wbu_difftest_ready")
}
