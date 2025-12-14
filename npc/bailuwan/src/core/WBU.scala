// Copyright (c) 2025 caozhanhao
// SPDX-License-Identifier: MIT

package core

import chisel3._
import chisel3.util._
import constants._
import utils.SignalProbe
import bailuwan.CoreParams

class WBURegfileOut(
  implicit p: CoreParams)
    extends Bundle {
  val rd_addr = UInt(5.W)
  val rd_data = UInt(p.XLEN.W)
  val rd_we   = Bool()
}

class WBU(
  implicit p: CoreParams)
    extends Module {
  val io = IO(new Bundle {
    val in = Flipped(Decoupled(new LSUOut))

    // Regfile
    val regfile_out = Output(new WBURegfileOut)
  })

  val exu_out = io.in.bits.from_exu
  val lsu_out = io.in.bits.read_data

  import ExecType._
  val rd_data = MuxLookup(exu_out.src_type, 0.U)(
    Seq(
      ALU -> exu_out.alu_out,
      LSU -> lsu_out,
      CSR -> exu_out.csr_out
    )
  )

  io.regfile_out.rd_addr := io.in.bits.from_exu.rd_addr
  io.regfile_out.rd_data := rd_data
  io.regfile_out.rd_we   := io.in.valid && exu_out.rd_we

  io.in.ready := true.B

  if (p.Debug) {
    dontTouch(io.in.bits.pc)
    dontTouch(io.in.bits.inst)
  }

  // Exposed signals for difftest
  // Note that the width of a RegNext is not set based on the next or
  // init connections for Element types
//  val wbu_pc = Reg(UInt(p.XLEN.W))
//  wbu_pc := io.in.bits.pc
//  val wbu_inst = Reg(UInt(32.W))
//  wbu_inst := io.in.bits.inst
//  SignalProbe(wbu_pc, "wbu_pc")
//  SignalProbe(wbu_inst, "wbu_inst")
//  SignalProbe(RegNext(io.in.fire), "wbu_difftest_ready")
    SignalProbe(io.in.bits.pc, "wbu_pc")
    SignalProbe(io.in.bits.inst, "wbu_inst")
    SignalProbe(io.in.fire, "wbu_difftest_ready")
}
