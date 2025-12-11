// Copyright (c) 2025 caozhanhao
// SPDX-License-Identifier: MIT

package core

import chisel3._
import chisel3.util._
import constants._
import utils.SignalProbe
import bailuwan.CoreParams

class WBUOut(
  implicit p: CoreParams)
    extends Bundle {
  // PC
  val dnpc = UInt(p.XLEN.W)
}

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
    val in  = Flipped(Decoupled(new LSUOut))
    val out = Decoupled(new WBUOut)

    val regfile_out = Output(new WBURegfileOut)
  })

  val s_idle :: s_wait_ready :: Nil = Enum(2)

  val state = RegInit(s_idle)

  state := MuxLookup(state, s_idle)(
    Seq(
      s_idle       -> Mux(io.in.fire, s_wait_ready, s_idle),
      s_wait_ready -> Mux(io.out.fire, s_idle, s_wait_ready)
    )
  )

  io.in.ready  := state === s_idle
  io.out.valid := state === s_wait_ready

  val exu_out = RegEnable(io.in.bits.from_exu, io.in.fire)
  val lsu_out = RegEnable(io.in.bits.read_data, io.in.fire)

  val br_dnpc = Mux(exu_out.br_taken, exu_out.br_target, exu_out.snpc)
  val dnpc    = MuxLookup(exu_out.src_type, br_dnpc)(
    Seq(
      ExecType.ECall -> exu_out.csr_out,
      ExecType.MRet  -> exu_out.csr_out
    )
  )

  import ExecType._
  val rd_data = MuxLookup(exu_out.src_type, 0.U)(
    Seq(
      ALU -> exu_out.alu_out,
      LSU -> lsu_out,
      CSR -> exu_out.csr_out
    )
  )

  io.out.bits.dnpc       := dnpc
  io.regfile_out.rd_addr := io.in.bits.from_exu.rd_addr
  io.regfile_out.rd_data := rd_data
  io.regfile_out.rd_we   := (state === s_wait_ready) && exu_out.rd_we

  SignalProbe(dnpc, "dnpc")
  // Difftest got ready after each instruction is done
  SignalProbe(state === s_wait_ready, "difftest_ready")
}
