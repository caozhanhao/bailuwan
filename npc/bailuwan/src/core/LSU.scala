// Copyright (c) 2025 caozhanhao
// SPDX-License-Identifier: MIT

package core

import chisel3._
import chisel3.util._
import constants._
import utils.Utils._
import amba._
import utils.PerfCounter
import bailuwan.CoreParams

class LSUOut(
  implicit p: CoreParams)
    extends Bundle {
  val read_data = UInt(p.XLEN.W)
  // Forward from EXU
  val from_exu  = new EXUOutForWBU

  // Debug
  val pc   = if (p.Debug) Some(UInt(p.XLEN.W)) else None
  val inst = if (p.Debug) Some(UInt(32.W)) else None
}

class LSU(
  implicit p: CoreParams,
  axi_prop:   AXIProperty)
    extends Module {
  val io = IO(new Bundle {
    val in  = Flipped(Decoupled(new EXUOut))
    val out = Decoupled(new LSUOut)

    val mem = new AXI4()

    // Hazard
    val rd       = Output(UInt(5.W))
    val rd_valid = Output(Bool())
  })

  assert(p.XLEN == 32, s"LSU: Unsupported XLEN: ${p.XLEN.toString}");

  val req_addr = io.in.bits.lsu.lsu_addr
  val req_op   = io.in.bits.lsu.lsu_op
  val req_data = io.in.bits.lsu.lsu_store_data
  val wbu_info = io.in.bits.wbu

  // States
  val s_idle :: s_r_addr :: s_r_wait_mem :: s_w_addr :: s_w_wait_mem :: s_wait_ready :: Nil = Enum(6)

  val entry_state = MuxLookup(req_op, s_idle)(
    Seq(
      LSUOp.Nop -> s_wait_ready,
      LSUOp.LB  -> s_r_addr,
      LSUOp.LH  -> s_r_addr,
      LSUOp.LW  -> s_r_addr,
      LSUOp.LBU -> s_r_addr,
      LSUOp.LHU -> s_r_addr,
      LSUOp.SB  -> s_w_addr,
      LSUOp.SH  -> s_w_addr,
      LSUOp.SW  -> s_w_addr
    )
  )

  val state = RegInit(s_idle)
  state := MuxLookup(state, s_idle)(
    Seq(
      // ATTENTION: io.in.valid rather than `fire`.
      //            Because we want the request latched in the pipeline registers during
      //            the transaction.
      s_idle       -> Mux(io.in.valid, entry_state, s_idle),
      s_r_addr     -> Mux(io.mem.ar.fire, s_r_wait_mem, s_r_addr),
      s_r_wait_mem -> Mux(io.mem.r.fire, s_wait_ready, s_r_wait_mem),
      s_w_addr     -> Mux(io.mem.aw.fire, s_w_wait_mem, s_w_addr),
      s_w_wait_mem -> Mux(io.mem.b.fire, s_wait_ready, s_w_wait_mem),
      s_wait_ready -> Mux(io.out.fire, s_idle, s_wait_ready)
    )
  )

  io.out.valid := state === s_wait_ready
  // Assert ready when we are returning to `s_idle`
  io.in.ready  := (state === s_wait_ready) && io.out.fire

  // EXU Forward
  io.out.bits.from_exu := wbu_info

  // Read
  io.mem.ar.bits.addr := req_addr
  io.mem.ar.valid     := state === s_r_addr
  io.mem.r.ready      := state === s_r_wait_mem

  val read_reg = Reg(UInt(32.W))
  read_reg := Mux(io.mem.r.fire, io.mem.r.bits.data, read_reg)

  val lb_sel = MuxLookup(req_addr(1, 0), 0.U(8.W))(
    Seq(
      0.U -> read_reg(7, 0),
      1.U -> read_reg(15, 8),
      2.U -> read_reg(23, 16),
      3.U -> read_reg(31, 24)
    )
  )

  val lh_sel = MuxLookup(req_addr(1, 0), 0.U(16.W))(
    Seq(
      0.U -> read_reg(15, 0),
      2.U -> read_reg(31, 16)
    )
  )

  val selected_loaded_data = MuxLookup(req_op, 0.U(p.XLEN.W))(
    Seq(
      LSUOp.LB  -> sign_extend(lb_sel, p.XLEN),
      LSUOp.LH  -> sign_extend(lh_sel, p.XLEN),
      LSUOp.LW  -> read_reg,
      LSUOp.LBU -> zero_extend(lb_sel, p.XLEN),
      LSUOp.LHU -> zero_extend(lh_sel, p.XLEN)
    )
  )

  io.mem.ar.bits.size := MuxLookup(req_op, 0.U(3.W))(
    Seq(
      LSUOp.LB  -> 0.U(3.W),
      LSUOp.LH  -> 1.U(3.W),
      LSUOp.LW  -> 2.U(3.W),
      LSUOp.LBU -> 0.U(3.W),
      LSUOp.LHU -> 1.U(3.W)
    )
  )

  io.out.bits.read_data := selected_loaded_data

  // Store
  val write_mask = MuxLookup(req_op, 0.U(4.W))(
    Seq(
      LSUOp.SB -> (0x1.U(4.W) << req_addr(1, 0)).asUInt,
      LSUOp.SH -> (0x3.U(4.W) << req_addr(1, 0)).asUInt,
      LSUOp.SW -> 0xf.U(4.W)
    )
  )

  val selected_store_data = MuxLookup(req_op, 0.U(p.XLEN.W))(
    Seq(
      LSUOp.SB -> (req_data << (req_addr(1, 0) << 3).asUInt).asUInt,
      LSUOp.SH -> (req_data << (req_addr(1, 0) << 3).asUInt).asUInt,
      LSUOp.SW -> req_data
    )
  )

  io.mem.aw.bits.size := MuxLookup(req_op, 0.U(3.W))(
    Seq(
      LSUOp.SB -> 0.U(3.W),
      LSUOp.SH -> 1.U(3.W),
      LSUOp.SW -> 2.U(3.W)
    )
  )

  io.mem.aw.bits.addr := req_addr
  io.mem.aw.valid     := state === s_w_addr
  io.mem.w.bits.data  := selected_store_data
  io.mem.w.bits.strb  := write_mask
  io.mem.w.valid      := state === s_w_addr || state === s_w_wait_mem
  io.mem.b.ready      := state === s_w_wait_mem

  io.mem.ar.bits.id    := 0.U
  io.mem.ar.bits.len   := 0.U // burst length=1, equivalent to an AxLEN value of zero.
  io.mem.ar.bits.burst := 0.U

  io.mem.aw.bits.id    := 0.U
  io.mem.aw.bits.len   := 0.U
  io.mem.aw.bits.burst := 0.U

  io.mem.w.bits.last := true.B

  // Hazard
  io.rd       := wbu_info.rd_addr
  io.rd_valid := io.in.valid && wbu_info.rd_we

  // Optional Debug Signals
  io.out.bits.pc.foreach { i => i := RegEnable(io.in.bits.lsu.pc.get, io.in.fire) }
  io.out.bits.inst.foreach { i => i := RegEnable(io.in.bits.lsu.inst.get, io.in.fire) }

  // Debug
  val misaligned = MuxLookup(req_op, false.B)(
    Seq(
      LSUOp.LH  -> req_addr(0),
      LSUOp.SH  -> req_addr(0),
      LSUOp.LHU -> req_addr(0),
      LSUOp.LW  -> (req_addr(1) | req_addr(0)),
      LSUOp.SW  -> (req_addr(1) | req_addr(0))
    )
  )

  assert(!misaligned, cf"LSU: Misaligned access at 0x${req_addr}%x")
  assert(
    !io.mem.r.valid || io.mem.r.bits.resp === AXIResp.OKAY,
    cf"LSU: Read fault at 0x${RegEnable(req_addr, io.mem.ar.fire)}%x, resp=${io.mem.r.bits.resp}"
  )
  assert(
    !io.mem.b.valid || io.mem.b.bits.resp === AXIResp.OKAY,
    cf"LSU: Write fault at 0x${RegEnable(req_addr, io.mem.aw.fire)}%x, resp=${io.mem.b.bits.resp}"
  )

  PerfCounter(io.mem.r.fire, "lsu_read")
  PerfCounter(io.mem.b.fire, "lsu_write")
}
