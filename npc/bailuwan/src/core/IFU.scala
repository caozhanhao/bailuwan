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
