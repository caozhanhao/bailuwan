// Copyright (c) 2025 caozhanhao
// SPDX-License-Identifier: MIT

package core

import amba._
import bailuwan.CoreParams
import chisel3._
import chisel3.util._
import utils.PerfCounter

class ICacheReq[T <: Data](
  gen:        => T
)(
  implicit p: CoreParams)
    extends Bundle {
  val addr = UInt(p.XLEN.W)
  val user = gen
}

class ICacheResp[T <: Data](
  gen:        => T
)(
  implicit p: CoreParams)
    extends Bundle {
  val data  = UInt(32.W)
  val addr  = Output(UInt(p.XLEN.W))
  val error = Bool()
  val user  = gen
}

class ICacheIO[T <: Data](
  gen:        => T
)(
  implicit p: CoreParams)
    extends Bundle {
  val req  = Decoupled(new ICacheReq(gen))
  val resp = Flipped(Decoupled(new ICacheResp(gen)))
  val kill = Bool()
}

class ICache[T <: Data](
  gen:        => T
)(
  implicit p: CoreParams,
  axi_prop:   AXIProperty)
    extends Module {
  val io = IO(new Bundle {
    val ifu   = Flipped(new ICacheIO(gen))
    val mem   = new AXI4()
    val flush = Input(Bool())
  })

  // Constants
  val BLOCK_BITS = 4 // 16-byte block
  val INDEX_BITS = 2 // 4 blocks

  // Calculated
  val WORDS_PER_BLOCK = 1 << (BLOCK_BITS - 2)
  val TAG_BITS        = 32 - INDEX_BITS - BLOCK_BITS
  val ENTRY_NUM       = 1 << INDEX_BITS

  // Request and Response
  val req  = io.ifu.req
  val resp = io.ifu.resp

  // States
  val s_idle :: s_fill_addr :: s_wait_mem :: s_resp :: Nil = Enum(4)

  val state = RegInit(s_idle)

  // Request Info
  val req_addr   = req.bits.addr
  val req_tag    = req_addr(31, INDEX_BITS + BLOCK_BITS)
  val req_index  = req_addr(INDEX_BITS + BLOCK_BITS - 1, BLOCK_BITS)
  val req_offset = if (BLOCK_BITS > 2) req_addr(BLOCK_BITS - 1, 2) else 0.U

  // Fill Info
  val fill_addr   = RegEnable(req_addr, req.fire)
  val fill_user   = RegEnable(req.bits.user, req.fire)
  val fill_tag    = fill_addr(31, INDEX_BITS + BLOCK_BITS)
  val fill_index  = fill_addr(INDEX_BITS + BLOCK_BITS - 1, BLOCK_BITS)
  val fill_offset = if (BLOCK_BITS > 2) fill_addr(BLOCK_BITS - 1, 2) else 0.U

  val fill_cnt = RegInit(0.U(log2Ceil(WORDS_PER_BLOCK).W))
  fill_cnt := Mux(state === s_idle, 0.U, Mux(io.mem.r.fire, fill_cnt + 1.U, fill_cnt))

  val fill_done = io.mem.r.fire && io.mem.r.bits.last

  // Cache Storage
  val valid_storage = RegInit(VecInit(Seq.fill(ENTRY_NUM)(false.B)))
  val tag_storage   = Reg(Vec(ENTRY_NUM, UInt(TAG_BITS.W)))
  val data_storage  = Reg(Vec(ENTRY_NUM, Vec(WORDS_PER_BLOCK, UInt(32.W))))

  // Entry Info Selected by Request
  val read_index  = Mux(state === s_idle, req_index, fill_index)
  val read_offset = Mux(state === s_idle, req_offset, fill_offset)
  val entry_valid = valid_storage(read_index)
  val entry_tag   = tag_storage(read_index)
  val entry_block = data_storage(read_index)
  val entry_data  = entry_block(read_offset)

  val hit = !io.flush && entry_valid && (entry_tag === req_tag)

  // We can't handle kill immediately in `s_wait_mem`
  val req_killed = RegInit(false.B)
  req_killed := MuxCase(req_killed, Seq(io.ifu.kill -> true.B, (state === s_idle) -> false.B))

  val is_killed = io.ifu.kill || req_killed

  // State Transfer
  state := MuxLookup(state, s_idle)(
    Seq(
      s_idle      -> Mux(
        // ATTENTION: use ifu.kill here not req_killed.
        //            Because req_killed will be set to false the next cycle after s_idle,
        //            but ICache is already ok to handle requests the first cycle in s_idle.
        req.fire && !io.ifu.kill,
        Mux(hit, Mux(resp.fire, s_idle, s_resp), Mux(io.mem.ar.fire, s_wait_mem, s_fill_addr)),
        s_idle
      ),
      // ATTENTION: If we've asserted ar.valid, we can NOT deassert it until ar.fire
      s_fill_addr -> Mux(io.mem.ar.fire, s_wait_mem, s_fill_addr),
      s_wait_mem  -> Mux(fill_done, Mux(is_killed, s_idle, s_resp), s_wait_mem),
      s_resp      -> Mux(resp.fire || is_killed, s_idle, s_resp)
    )
  )

  // Fill
  valid_storage.zipWithIndex.foreach { case (r, i) =>
    r := Mux(io.flush, false.B, Mux(fill_done && (fill_index === i.U), true.B, r))
  }
  tag_storage(fill_index) := Mux(fill_done, fill_tag, tag_storage(fill_index))

  data_storage(fill_index)(fill_cnt) :=
    Mux(io.mem.r.fire, io.mem.r.bits.data, data_storage(fill_index)(fill_cnt))

  val err      = RegInit(false.B)
  val curr_err = io.mem.r.bits.resp =/= AXIResp.OKAY
  err := Mux(state === s_idle, false.B, Mux(io.mem.r.fire, err || curr_err, err))

  // IFU IO
  // Immediate hit or s_resp
  resp.valid      := ((req.fire && hit) || (state === s_resp)) && !is_killed
  resp.bits.data  := entry_data
  resp.bits.addr  := Mux(state === s_idle, req_addr, fill_addr)
  resp.bits.user  := Mux(state === s_idle, req.bits.user, fill_user)
  resp.bits.error := err
  req.ready       := state === s_idle

  // Mem IO
  io.mem.ar.valid := state === s_fill_addr
  val block_align_mask = (~((1 << BLOCK_BITS) - 1).U(32.W)).asUInt
  io.mem.ar.bits.addr := fill_addr & block_align_mask

  io.mem.r.ready := state === s_wait_mem

  io.mem.ar.bits.id    := 0.U
  io.mem.ar.bits.len   := (WORDS_PER_BLOCK - 1).U
  io.mem.ar.bits.size  := 2.U // 2^2 = 4 bytes
  io.mem.ar.bits.burst := AXIBurstType.INCR
  io.mem.aw.valid      := false.B
  io.mem.aw.bits       := DontCare
  io.mem.w.valid       := false.B
  io.mem.w.bits        := DontCare
  io.mem.b.ready       := false.B
  io.mem.w.bits.last   := true.B

  PerfCounter(state === s_idle && req.valid && resp.ready && hit, "icache_hit")
  PerfCounter(state === s_idle && req.valid && !hit, "icache_miss")
  PerfCounter(state === s_wait_mem, "icache_mem_access_cycles")
}
