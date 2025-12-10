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
}

class LSU(
  implicit p: CoreParams,
  axi_prop:   AXIProperty)
    extends Module {
  val io = IO(new Bundle {
    val in  = Flipped(Decoupled(new EXUOut))
    val out = Decoupled(new LSUOut)

    val mem = new AXI4()
  })

  io.mem.ar.bits.id    := 0.U
  io.mem.ar.bits.len   := 0.U // burst length=1, equivalent to an AxLEN value of zero.
  io.mem.ar.bits.burst := 0.U

  io.mem.aw.bits.id    := 0.U
  io.mem.aw.bits.len   := 0.U // burst length=1, equivalent to an AxLEN value of zero.
  io.mem.aw.bits.burst := 0.U

  io.mem.w.bits.last := true.B

  assert(p.XLEN == 32, s"LSU: Unsupported XLEN: ${p.XLEN.toString}");

  val addr       = RegEnable(io.in.bits.lsu.lsu_addr, io.in.fire)
  val op         = RegEnable(io.in.bits.lsu.lsu_op, io.in.fire)
  val store_data = RegEnable(io.in.bits.lsu.lsu_store_data, io.in.fire)

  // Read
  val enter_read = io.in.fire && MuxLookup(op, false.B)(
    Seq(
      LSUOp.LB  -> true.B,
      LSUOp.LH  -> true.B,
      LSUOp.LW  -> true.B,
      LSUOp.LBU -> true.B,
      LSUOp.LHU -> true.B
    )
  )

  val r_idle :: r_ar :: r_wait_mem :: r_wait_ready :: Nil = Enum(4)

  val r_state = RegInit(r_idle)
  r_state := MuxLookup(r_state, r_idle)(
    Seq(
      r_idle       -> Mux(enter_read, r_ar, r_idle),
      r_ar         -> Mux(io.mem.ar.fire, r_wait_mem, r_ar),
      r_wait_mem   -> Mux(io.mem.r.fire, r_wait_ready, r_wait_mem),
      r_wait_ready -> Mux(io.out.fire, r_idle, r_wait_ready)
    )
  )

  io.mem.ar.bits.addr := addr
  io.mem.ar.valid     := r_state === r_ar
  io.mem.r.ready      := r_state === r_wait_mem

  val read_reg = Reg(UInt(32.W))
  read_reg := Mux(io.mem.r.valid, io.mem.r.bits.data, read_reg)

  val lb_sel = MuxLookup(addr(1, 0), 0.U(8.W))(
    Seq(
      0.U -> read_reg(7, 0),
      1.U -> read_reg(15, 8),
      2.U -> read_reg(23, 16),
      3.U -> read_reg(31, 24)
    )
  )

  val lh_sel = MuxLookup(addr(1, 0), 0.U(16.W))(
    Seq(
      0.U -> read_reg(15, 0),
      2.U -> read_reg(31, 16)
    )
  )

  val selected_loaded_data = MuxLookup(op, 0.U(p.XLEN.W))(
    Seq(
      LSUOp.LB  -> sign_extend(lb_sel, p.XLEN),
      LSUOp.LH  -> sign_extend(lh_sel, p.XLEN),
      LSUOp.LW  -> read_reg,
      LSUOp.LBU -> zero_extend(lb_sel, p.XLEN),
      LSUOp.LHU -> zero_extend(lh_sel, p.XLEN)
    )
  )

  io.mem.ar.bits.size := MuxLookup(op, 0.U(3.W))(
    Seq(
      LSUOp.LB  -> 0.U(3.W),
      LSUOp.LH  -> 1.U(3.W),
      LSUOp.LW  -> 2.U(3.W),
      LSUOp.LBU -> 0.U(3.W),
      LSUOp.LHU -> 1.U(3.W)
    )
  )

  io.out.bits.read_data := selected_loaded_data

  // Write
  val w_idle :: w_aw :: w_wait_mem :: w_wait_ready :: Nil = Enum(4)

  val w_state     = RegInit(w_idle)
  val enter_write = io.in.fire && MuxLookup(op, false.B)(
    Seq(
      LSUOp.SB -> true.B,
      LSUOp.SH -> true.B,
      LSUOp.SW -> true.B
    )
  )

  w_state := MuxLookup(w_state, w_idle)(
    Seq(
      w_idle       -> Mux(enter_write, w_aw, w_idle),
      w_aw         -> Mux(io.mem.aw.fire, w_wait_mem, w_aw),
      w_wait_mem   -> Mux(io.mem.b.fire, w_wait_ready, w_wait_mem),
      w_wait_ready -> Mux(io.out.fire, w_idle, w_wait_ready)
    )
  )

  val write_mask = MuxLookup(op, 0.U(4.W))(
    Seq(
      LSUOp.SB -> (0x1.U(4.W) << addr(1, 0)).asUInt,
      LSUOp.SH -> (0x3.U(4.W) << addr(1, 0)).asUInt,
      LSUOp.SW -> 0xf.U(4.W)
    )
  )

  val selected_store_data = MuxLookup(op, 0.U(p.XLEN.W))(
    Seq(
      LSUOp.SB -> (store_data << (addr(1, 0) << 3).asUInt).asUInt,
      LSUOp.SH -> (store_data << (addr(1, 0) << 3).asUInt).asUInt,
      LSUOp.SW -> store_data
    )
  )

  io.mem.aw.bits.size := MuxLookup(op, 0.U(3.W))(
    Seq(
      LSUOp.SB -> 0.U(3.W),
      LSUOp.SH -> 1.U(3.W),
      LSUOp.SW -> 2.U(3.W)
    )
  )

  io.mem.aw.bits.addr := addr
  io.mem.aw.valid     := w_state === w_aw
  io.mem.w.bits.data  := selected_store_data
  io.mem.w.bits.strb  := write_mask
  io.mem.w.valid      := w_state === w_aw || w_state === w_wait_mem
  io.mem.b.ready      := w_state === w_wait_mem

  io.out.valid := (io.in.valid && op === LSUOp.Nop) || (r_state === r_wait_ready) || (w_state === w_wait_ready)
  io.in.ready  := (r_state === r_idle) && (w_state === w_idle) && io.out.ready

  // Forward EXU Signals
  io.out.bits.from_exu := io.in.bits.wbu

  // Debug
  val misaligned = MuxLookup(op, false.B)(
    Seq(
      LSUOp.LH  -> addr(0),
      LSUOp.SH  -> addr(0),
      LSUOp.LHU -> addr(0),
      LSUOp.LW  -> (addr(1) | addr(0)),
      LSUOp.SW  -> (addr(1) | addr(0))
    )
  )

  assert(!misaligned, cf"LSU: Misaligned access at 0x${addr}%x")
  assert(
    !io.mem.r.valid || io.mem.r.bits.resp === AXIResp.OKAY,
    cf"LSU: Read fault at 0x${RegEnable(addr, io.mem.ar.fire)}%x, resp=${io.mem.r.bits.resp}"
  )
  assert(
    !io.mem.b.valid || io.mem.b.bits.resp === AXIResp.OKAY,
    cf"LSU: Write fault at 0x${RegEnable(addr, io.mem.aw.fire)}%x, resp=${io.mem.b.bits.resp}"
  )

  PerfCounter(io.mem.r.fire, "lsu_read")
}
