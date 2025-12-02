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

class ICachePlaceHolder(
  implicit p: CoreParams,
  axi_prop:   AXIProperty)
    extends Module {
  val io   = IO(new Bundle {
    val ifu = Flipped(new ICacheIO())
    val mem = new AXI4()
  })
  val req  = io.ifu.req
  val resp = io.ifu.resp

  io.mem.ar.valid     := req.valid
  req.ready           := io.mem.ar.ready
  io.mem.ar.bits.addr := req.bits.addr

  io.mem.r.ready  := resp.ready
  resp.valid      := io.mem.r.valid
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
  // 1-bit valid | TAG_BITS-bit tag | (1 << BLOCK_BITS)-bit data
  val ENTRY_BITS = 1 + TAG_BITS + (1 << BLOCK_BITS)

  // Request and Response
  val req  = io.ifu.req
  val resp = io.ifu.resp

  // Request Info
  val req_addr  = req.bits.addr
  val req_tag   = req_addr(31, INDEX_BITS + BLOCK_BITS)
  val req_index = req_addr(INDEX_BITS + BLOCK_BITS - 1, BLOCK_BITS)

  // Cache Storage
  val storage = RegInit(VecInit(Seq.fill(1 << INDEX_BITS)(0.U(ENTRY_BITS.W))))

  // Entry Info Selected by Request
  val entry       = storage(req_index)
  val entry_valid = entry(ENTRY_BITS - 1)
  val entry_tag   = entry(ENTRY_BITS - 1, TAG_BITS + (1 << BLOCK_BITS))
  val entry_data  = entry((1 << BLOCK_BITS) - 1, 0)

  val hit = entry_valid && (entry_tag === req_tag)

  // Fill Info
  val fill_addr  = RegInit(0.U(32.W))
  val fill_tag   = fill_addr(31, INDEX_BITS + BLOCK_BITS)
  val fill_index = fill_addr(INDEX_BITS + BLOCK_BITS - 1, BLOCK_BITS)

  fill_addr := Mux(req.valid, req_addr, fill_addr)

  // States
  val s_idle :: s_fill :: s_wait_mem :: Nil = Enum(3)

  val state = RegInit(s_idle)
  state := MuxLookup(state, s_idle)(
    Seq(
      s_idle     -> Mux(req.valid, Mux(hit, s_idle, s_fill), s_idle),
      s_fill     -> Mux(io.mem.ar.fire, s_wait_mem, s_fill),
      s_wait_mem -> Mux(io.mem.r.fire, s_idle, s_wait_mem)
    )
  )

  // Fill
  val new_entry = true.B ## fill_tag ## io.mem.r.bits.data
  storage(fill_index) := Mux(io.mem.r.fire, new_entry, storage(fill_index))

  // IFU IO
  // Immediate hit or refill+hit
  resp.valid      := req.valid && hit
  resp.bits.data  := entry_data
  resp.bits.error := false.B // TODO

  req.ready := state === s_idle

  // Mem IO
  io.mem.ar.valid     := state === s_fill
  io.mem.ar.bits.addr := fill_addr

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
