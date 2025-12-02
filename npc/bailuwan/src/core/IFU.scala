// Copyright (c) 2025 caozhanhao
// SPDX-License-Identifier: MIT

package core

import chisel3._
import chisel3.util._
import amba._
import utils.{PerfCounter, SignalProbe}
import bailuwan.CoreParams

class IFUOut(
  implicit p: CoreParams)
    extends Bundle {
  val pc   = Output(UInt(p.XLEN.W))
  val inst = Output(UInt(32.W))
}

class ICacheReq(
  implicit p: CoreParams)
    extends Bundle {
  val addr = UInt(p.XLEN.W)
}

class ICacheResp(
  implicit p: CoreParams)
    extends Bundle {
  val data  = UInt(32.W)
  val error = Bool()
}

class ICacheIO(
  implicit p: CoreParams)
    extends Bundle {
  val req  = Decoupled(new ICacheReq)
  val resp = Flipped(Decoupled(new ICacheResp))
}

class ICache(
  implicit p: CoreParams,
  axi_prop:   AXIProperty)
    extends Module {
  val io = IO(new Bundle {
    val ifu = Flipped(new ICacheIO())
    val mem = new AXI4()
  })

  // Constants
  val BLOCK_BITS = 2 // 4-byte block
  val INDEX_BITS = 4 // 16 blocks

  val TAG_BITS   = 31 - INDEX_BITS - BLOCK_BITS
  // 1-bit valid | TAG_BITS-bit tag | DATA_BITS-bit data
  val DATA_BITS  = (1 << BLOCK_BITS) * 8
  val ENTRY_BITS = 1 + TAG_BITS + DATA_BITS
  val ENTRY_NUM  = 1 << INDEX_BITS

  // Request and Response
  val req  = io.ifu.req
  val resp = io.ifu.resp

  // States
  val s_idle :: s_fill :: s_wait_mem :: s_resp :: Nil = Enum(4)

  val state = RegInit(s_idle)

  // Request Info
  val req_addr  = req.bits.addr
  val req_tag   = req_addr(31, INDEX_BITS + BLOCK_BITS)
  val req_index = req_addr(INDEX_BITS + BLOCK_BITS - 1, BLOCK_BITS)

  // Fill Info
  val fill_addr  = RegInit(0.U(32.W))
  val fill_tag   = fill_addr(31, INDEX_BITS + BLOCK_BITS)
  val fill_index = fill_addr(INDEX_BITS + BLOCK_BITS - 1, BLOCK_BITS)

  fill_addr := Mux(state === s_idle && req.valid, req_addr, fill_addr)

  // Cache Storage
  val valid_storage = RegInit(VecInit(Seq.fill(ENTRY_NUM)(false.B)))
  val tag_storage   = Reg(Vec(ENTRY_NUM, UInt(TAG_BITS.W)))
  val data_storage  = Reg(Vec(ENTRY_NUM, UInt(DATA_BITS.W)))

  // Entry Info Selected by Request
  val read_index  = Mux(state === s_idle, req_index, fill_index)
  val entry_valid = valid_storage(read_index)
  val entry_tag   = tag_storage(read_index)
  val entry_data  = data_storage(read_index)

  val hit = entry_valid && (entry_tag === req_tag)

  // State Transfer
  state := MuxLookup(state, s_idle)(
    Seq(
      s_idle     -> Mux(req.valid, Mux(hit, s_idle, s_fill), s_idle),
      s_fill     -> Mux(io.mem.ar.fire, s_wait_mem, s_fill),
      s_wait_mem -> Mux(io.mem.r.fire, Mux(resp.ready, s_idle, s_resp), s_wait_mem),
      s_resp     -> Mux(resp.fire, s_idle, s_resp)
    )
  )

  // Fill
  valid_storage(fill_index) := Mux(io.mem.r.fire, true.B, valid_storage(fill_index))
  tag_storage(fill_index)   := Mux(io.mem.r.fire, fill_tag, tag_storage(fill_index))
  data_storage(fill_index)  := Mux(io.mem.r.fire, io.mem.r.bits.data, data_storage(fill_index))

  val err      = RegInit(false.B)
  val curr_err = io.mem.r.bits.resp =/= AXIResp.OKAY
  err := Mux(io.mem.r.fire, curr_err, err)

  // IFU IO
  // Immediate hit or s_resp
  val resp_bypass = state === s_wait_mem && io.mem.r.valid
  resp.valid      := (req.valid && hit) || (state === s_resp) || resp_bypass
  resp.bits.data  := Mux(resp_bypass, io.mem.r.bits.data, entry_data)
  resp.bits.error := Mux(resp_bypass, curr_err, err)

  req.ready := state === s_idle

  // Ensure (req.valid && hit) => resp.ready.
  // If the IFU sends a request that hits the cache but is not ready to receive
  // data in the same cycle, the request will be lost because we do NOT latch
  // hit responses. Thus, we assert this requirement here.
  assert(!(req.valid && hit) || resp.ready)

  // Mem IO
  val ar_bypass = state === s_idle && req.valid && !hit
  io.mem.ar.valid     := ar_bypass || (state === s_fill)
  io.mem.ar.bits.addr := Mux(ar_bypass, req.bits.addr, fill_addr)

  io.mem.r.ready := state === s_wait_mem

  io.mem.ar.bits.id    := 0.U
  io.mem.ar.bits.len   := 0.U // burst length=1, equivalent to an AxLEN value of zero.
  io.mem.ar.bits.size  := 2.U // 2^2 = 4 bytes
  io.mem.ar.bits.burst := 0.U
  io.mem.aw.valid      := false.B
  io.mem.aw.bits       := DontCare
  io.mem.w.valid       := false.B
  io.mem.w.bits        := DontCare
  io.mem.b.ready       := false.B
  io.mem.w.bits.last   := true.B

  PerfCounter(state === s_idle && req.valid && hit, "icache_hit")
  PerfCounter(state === s_idle && req.valid && !hit, "icache_miss")
}

class ICachePlaceholder(
  implicit p: CoreParams,
  axi_prop:   AXIProperty)
    extends Module {
  val io   = IO(new Bundle {
    val ifu = Flipped(new ICacheIO())
    val mem = new AXI4()
  })
  val req  = io.ifu.req
  val resp = io.ifu.resp

  req.ready           := io.mem.ar.ready
  io.mem.ar.valid     := req.valid
  io.mem.ar.bits.addr := req.bits.addr

  resp.valid      := io.mem.r.valid
  io.mem.r.ready  := resp.ready
  resp.bits.data  := io.mem.r.bits.data
  resp.bits.error := io.mem.r.bits.resp =/= AXIResp.OKAY

  io.mem.ar.bits.id    := 0.U
  io.mem.ar.bits.len   := 0.U // burst length=1, equivalent to an AxLEN value of zero.
  io.mem.ar.bits.size  := 2.U // 2^2 = 4 bytes
  io.mem.ar.bits.burst := 0.U
  io.mem.aw.valid      := false.B
  io.mem.aw.bits       := DontCare
  io.mem.w.valid       := false.B
  io.mem.w.bits        := DontCare
  io.mem.b.ready       := false.B
  io.mem.w.bits.last   := true.B
}

class IFU(
  implicit p: CoreParams,
  axi_prop:   AXIProperty)
    extends Module {
  val io = IO(new Bundle {
    val in  = Flipped(Decoupled(new WBUOut))
    val out = Decoupled(new IFUOut)

    val mem = new AXI4()
  })

  val icache    = Module(new ICache)
  val icache_io = icache.io.ifu

  icache.io.mem <> io.mem

  val s_idle :: s_wait_mem :: s_wait_ready :: s_fault :: Nil = Enum(4)

  val state = RegInit(s_idle)
  state := MuxLookup(state, s_idle)(
    Seq(
      s_idle       -> Mux(icache_io.req.fire, s_wait_mem, s_idle),
      s_wait_mem   -> Mux(icache_io.resp.fire, Mux(icache_io.resp.bits.error, s_fault, s_wait_ready), s_wait_mem),
      s_wait_ready -> Mux(io.out.fire, s_idle, s_wait_ready)
    )
  )

  icache_io.req.valid  := (state === s_idle) && !reset.asBool // Don't send request when resetting
  icache_io.resp.ready := state === s_wait_mem

  val pc = RegInit(p.ResetVector.S(p.XLEN.W).asUInt)
  pc := Mux(io.in.fire, io.in.bits.dnpc, pc)

  icache_io.req.bits.addr := pc

  val inst_reg = RegInit(0.U(32.W))
  inst_reg := Mux(icache_io.resp.fire, icache_io.resp.bits.data, inst_reg)

  io.out.bits.inst := inst_reg
  io.out.bits.pc   := pc

  io.in.ready  := io.out.ready
  io.out.valid := state === s_wait_ready

  val fault_addr = RegInit(0.U(p.XLEN.W))
  fault_addr := Mux(icache_io.req.fire, pc, fault_addr)

  assert(state =/= s_fault, cf"IFU: Access fault at 0x${fault_addr}%x")

  // Difftest got ready after every pc advance (one instruction done),
  // which is just in.valid delayed one cycle.
  //               ___________
  //   ready      |          |
  //              _____       _____
  //   clock     |     |_____|     |_____
  //              cycle 1        cycle 2
  //                     ^
  //                     |
  //          difftest_step is called here
  SignalProbe(RegNext(io.in.valid), "difftest_ready")
  SignalProbe(pc, "pc")
  SignalProbe(state, "ifu_state")
  PerfCounter(icache_io.resp.fire, "ifu_fetched")
}
