// Copyright (c) 2025-2026 caozhanhao
// SPDX-License-Identifier: MulanPSL-2.0

package amba

import chisel3._
import chisel3.util._

class AXI4ErrorSlave(
  implicit p: AXIProperty)
    extends Module {
  val io = IO(Flipped(new AXI4))

  val r_idle :: r_busy :: Nil = Enum(2)

  val r_state = RegInit(r_idle)
  val r_id    = Reg(UInt(p.ID_R_WIDTH.W))
  val r_len   = Reg(UInt(8.W))
  val r_cnt   = Reg(UInt(8.W))

  r_state := MuxLookup(r_state, r_idle)(
    Seq(
      r_idle -> Mux(io.ar.fire, r_busy, r_idle),
      r_busy -> Mux(io.r.fire && io.r.bits.last, r_idle, r_busy)
    )
  )

  r_id  := Mux(io.ar.fire, io.ar.bits.id, r_id)
  r_len := Mux(io.ar.fire, io.ar.bits.len, r_len)
  r_cnt := MuxCase(
    0.U,
    Seq(
      io.ar.fire -> io.ar.bits.len,
      io.r.fire  -> (r_cnt - 1.U)
    )
  )

  io.ar.ready    := r_state === r_idle
  io.r.valid     := r_state === r_busy
  io.r.bits.data := 25100251.U
  io.r.bits.resp := AXIResp.DECERR
  io.r.bits.id   := r_id
  io.r.bits.last := r_cnt === 0.U

  val w_idle :: w_wait_data :: w_resp :: Nil = Enum(3)

  val w_state = RegInit(w_idle)
  val w_id    = Reg(UInt(p.ID_W_WIDTH.W))

  w_state := MuxLookup(w_state, w_idle)(
    Seq(
      w_idle      -> Mux(io.aw.fire, w_wait_data, w_idle),
      w_wait_data -> Mux(io.w.fire && io.w.bits.last, w_resp, w_wait_data),
      w_resp      -> Mux(io.b.fire, w_idle, w_resp)
    )
  )

  io.aw.ready    := w_state === w_idle
  io.w.ready     := w_state === w_wait_data
  io.b.valid     := w_state === w_resp
  io.b.bits.resp := AXIResp.DECERR
  io.b.bits.id   := w_id
}

class AXI4CrossBar(
  val slaves_map: Seq[Seq[(Long, Long)]]
)(
  implicit p:     AXIProperty)
    extends Module {
  val io = IO(new Bundle {
    val master = Flipped(new AXI4)
    val slaves = Vec(slaves_map.size, new AXI4)
  })
  assert(slaves_map.size > 1, "Crossbar for what?")

  val n         = slaves_map.size
  val idx_width = math.max(1, log2Ceil(n + 1)) // reserve 1 for err
  val err_idx   = n.U
  val master    = io.master

  val err_slave = Module(new AXI4ErrorSlave)
  val slaves    = Wire(Vec(n + 1, new AXI4))
  for (i <- 0 until n) {
    io.slaves(i) <> slaves(i)
  }
  err_slave.io <> slaves(n)

  // First block all slaves (including the error slave)
  for (i <- 0 until n + 1) {
    val s = slaves(i)
    s.ar.valid := false.B
    s.ar.bits  := 0.U.asTypeOf(s.ar.bits)

    s.aw.valid := false.B
    s.aw.bits  := 0.U.asTypeOf(s.aw.bits)

    s.w.valid := false.B
    s.w.bits  := 0.U.asTypeOf(s.w.bits)

    s.r.ready := false.B
    s.b.ready := false.B
  }

  // Address Mux
  def addr_mux_map(addr: UInt) = slaves_map.zipWithIndex.map(x => {
    val ranges = x._1
    val idx    = x._2.asUInt

    val matches = VecInit(ranges.map(rng => {
      val (lo, hi) = rng
      lo.S.asUInt <= addr && addr < hi.S.asUInt
    })).asUInt

    matches.orR -> idx
  })

  // Read Crossbar
  val r_idle :: r_busy :: Nil = Enum(2)

  val r_state = RegInit(r_idle)

  val r_candidate = MuxCase(err_idx, addr_mux_map(master.ar.bits.addr))

  val r_owner_id = RegInit(0.U(idx_width.W))
  r_owner_id := Mux(r_state === r_idle && master.ar.valid, r_candidate, r_owner_id)
  val r_owner = slaves(r_owner_id)

  r_state := MuxLookup(r_state, r_idle)(
    Seq(
      r_idle -> Mux(master.ar.valid, r_busy, r_idle),
      r_busy -> Mux(r_owner.r.fire && r_owner.r.bits.last, r_idle, r_busy)
    )
  )

  def if_rbusy[T <: Data](x: T) =
    Mux(r_state === r_busy, x, 0.U.asTypeOf(x))

  // Connect ar
  r_owner.ar.valid := if_rbusy(master.ar.valid)
  r_owner.ar.bits  := if_rbusy(master.ar.bits)
  master.ar.ready  := if_rbusy(r_owner.ar.ready)

  // printf(cf"Connected: ${r_owner_id}, ar valid: ${r_owner.ar.valid}, ar bits: ${r_owner.ar.bits.addr}\n")

  // Connect r
  master.r.bits   := if_rbusy(r_owner.r.bits)
  master.r.valid  := if_rbusy(r_owner.r.valid)
  r_owner.r.ready := if_rbusy(master.r.ready)

  // Write Crossbar
  val w_idle :: w_busy :: Nil = Enum(2)

  val w_state = RegInit(w_idle)

  val w_candidate = MuxCase(err_idx, addr_mux_map(master.aw.bits.addr))

  val w_owner_id = RegInit(0.U(idx_width.W))
  w_owner_id := Mux(w_state === w_idle && master.aw.valid, w_candidate, w_owner_id)
  val w_owner = slaves(w_owner_id)

  w_state := MuxLookup(w_state, w_idle)(
    Seq(
      w_idle -> Mux(master.aw.valid, w_busy, w_idle),
      w_busy -> Mux(w_owner.b.fire, w_idle, w_busy)
    )
  )

  def if_wbusy[T <: Data](x: T) =
    Mux(w_state === w_busy, x, 0.U.asTypeOf(x))

  // Connect aw
  w_owner.aw.valid := if_wbusy(master.aw.valid)
  w_owner.aw.bits  := if_wbusy(master.aw.bits)
  master.aw.ready  := if_wbusy(w_owner.aw.ready)

  // Connect w
  w_owner.w.valid := if_wbusy(master.w.valid)
  w_owner.w.bits  := if_wbusy(master.w.bits)
  master.w.ready  := if_wbusy(w_owner.w.ready)

  // Connect b
  master.b.bits   := if_wbusy(w_owner.b.bits)
  master.b.valid  := if_wbusy(w_owner.b.valid)
  w_owner.b.ready := if_wbusy(master.b.ready)
}
