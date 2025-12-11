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

  assert(p.XLEN == 32, s"LSU: Unsupported XLEN: ${p.XLEN.toString}");

  val addr_reg       = RegEnable(io.in.bits.lsu.lsu_addr, io.in.fire)
  val op_reg         = RegEnable(io.in.bits.lsu.lsu_op, io.in.fire)
  val store_data_reg = RegEnable(io.in.bits.lsu.lsu_store_data, io.in.fire)

  // States
  val s_idle :: s_r_addr :: s_r_wait_mem :: s_w_addr :: s_w_wait_mem :: s_wait_ready :: Nil = Enum(6)

  // Don't use latched op here.
  val entry_state = MuxLookup(io.in.bits.lsu.lsu_op, s_idle)(
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
      s_idle       -> Mux(io.in.fire, entry_state, s_idle),
      s_r_addr     -> Mux(io.mem.ar.fire, s_r_wait_mem, s_r_addr),
      s_r_wait_mem -> Mux(io.mem.r.fire, s_wait_ready, s_r_wait_mem),
      s_w_addr     -> Mux(io.mem.aw.fire, s_w_wait_mem, s_w_addr),
      s_w_wait_mem -> Mux(io.mem.b.fire, s_wait_ready, s_w_wait_mem),
      s_wait_ready -> Mux(io.out.fire, s_idle, s_wait_ready)
    )
  )

  io.out.valid := state === s_wait_ready
  io.in.ready  := state === s_idle

  // EXU Forward
  io.out.bits.from_exu := RegEnable(io.in.bits.wbu, io.in.fire)

  // Read
  io.mem.ar.bits.addr := addr_reg
  io.mem.ar.valid     := state === s_r_addr
  io.mem.r.ready      := state === s_r_wait_mem

  val read_reg = Reg(UInt(32.W))
  read_reg := Mux(io.mem.r.fire, io.mem.r.bits.data, read_reg)

  val lb_sel = MuxLookup(addr_reg(1, 0), 0.U(8.W))(
    Seq(
      0.U -> read_reg(7, 0),
      1.U -> read_reg(15, 8),
      2.U -> read_reg(23, 16),
      3.U -> read_reg(31, 24)
    )
  )

  val lh_sel = MuxLookup(addr_reg(1, 0), 0.U(16.W))(
    Seq(
      0.U -> read_reg(15, 0),
      2.U -> read_reg(31, 16)
    )
  )

  val selected_loaded_data = MuxLookup(op_reg, 0.U(p.XLEN.W))(
    Seq(
      LSUOp.LB  -> sign_extend(lb_sel, p.XLEN),
      LSUOp.LH  -> sign_extend(lh_sel, p.XLEN),
      LSUOp.LW  -> read_reg,
      LSUOp.LBU -> zero_extend(lb_sel, p.XLEN),
      LSUOp.LHU -> zero_extend(lh_sel, p.XLEN)
    )
  )

  io.mem.ar.bits.size := MuxLookup(op_reg, 0.U(3.W))(
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
  val write_mask = MuxLookup(op_reg, 0.U(4.W))(
    Seq(
      LSUOp.SB -> (0x1.U(4.W) << addr_reg(1, 0)).asUInt,
      LSUOp.SH -> (0x3.U(4.W) << addr_reg(1, 0)).asUInt,
      LSUOp.SW -> 0xf.U(4.W)
    )
  )

  val selected_store_data = MuxLookup(op_reg, 0.U(p.XLEN.W))(
    Seq(
      LSUOp.SB -> (store_data_reg << (addr_reg(1, 0) << 3).asUInt).asUInt,
      LSUOp.SH -> (store_data_reg << (addr_reg(1, 0) << 3).asUInt).asUInt,
      LSUOp.SW -> store_data_reg
    )
  )

  io.mem.aw.bits.size := MuxLookup(op_reg, 0.U(3.W))(
    Seq(
      LSUOp.SB -> 0.U(3.W),
      LSUOp.SH -> 1.U(3.W),
      LSUOp.SW -> 2.U(3.W)
    )
  )

  io.mem.aw.bits.addr := addr_reg
  io.mem.aw.valid     := state === s_w_addr
  io.mem.w.bits.data  := selected_store_data
  io.mem.w.bits.strb  := write_mask
  io.mem.w.valid      := state === s_w_wait_mem
  io.mem.b.ready      := state === s_w_wait_mem

  io.mem.ar.bits.id    := 0.U
  io.mem.ar.bits.len   := 0.U // burst length=1, equivalent to an AxLEN value of zero.
  io.mem.ar.bits.burst := 0.U

  io.mem.aw.bits.id    := 0.U
  io.mem.aw.bits.len   := 0.U
  io.mem.aw.bits.burst := 0.U

  io.mem.w.bits.last := true.B

  // Debug
  val misaligned = MuxLookup(op_reg, false.B)(
    Seq(
      LSUOp.LH  -> addr_reg(0),
      LSUOp.SH  -> addr_reg(0),
      LSUOp.LHU -> addr_reg(0),
      LSUOp.LW  -> (addr_reg(1) | addr_reg(0)),
      LSUOp.SW  -> (addr_reg(1) | addr_reg(0))
    )
  )

  assert(!misaligned, cf"LSU: Misaligned access at 0x${addr_reg}%x")
  assert(
    !io.mem.r.valid || io.mem.r.bits.resp === AXIResp.OKAY,
    cf"LSU: Read fault at 0x${RegEnable(addr_reg, io.mem.ar.fire)}%x, resp=${io.mem.r.bits.resp}"
  )
  assert(
    !io.mem.b.valid || io.mem.b.bits.resp === AXIResp.OKAY,
    cf"LSU: Write fault at 0x${RegEnable(addr_reg, io.mem.aw.fire)}%x, resp=${io.mem.b.bits.resp}"
  )

  PerfCounter(io.mem.r.fire, "lsu_read")
}
