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

class ICache(
  implicit p: CoreParams,
  axi_prop:   AXIProperty)
    extends Module {
  val io = IO(new Bundle {
    val ifu = Flipped(new AXI4())
    val mem = new AXI4()
  })
  io.ifu <> io.mem
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

  val icache = Module(new ICache)
  icache.io.mem <> io.mem

  val reader = icache.io.ifu

  reader.ar.bits.id    := 0.U
  reader.ar.bits.len   := 0.U // burst length=1, equivalent to an AxLEN value of zero.
  reader.ar.bits.size  := 2.U // 2^2 = 4 bytes
  reader.ar.bits.burst := 0.U

  reader.aw.valid    := false.B
  reader.aw.bits     := DontCare
  reader.w.valid     := false.B
  reader.w.bits      := DontCare
  reader.b.ready     := false.B
  reader.w.bits.last := true.B

  val s_idle :: s_wait_mem :: s_wait_ready :: s_fault :: Nil = Enum(4)

  val state = RegInit(s_idle)
  state := MuxLookup(state, s_idle)(
    Seq(
      s_idle       -> Mux(reader.ar.fire, s_wait_mem, s_idle),
      s_wait_mem   -> Mux(reader.r.fire, Mux(reader.r.bits.resp === AXIResp.OKAY, s_wait_ready, s_fault), s_wait_mem),
      s_wait_ready -> Mux(io.out.fire, s_idle, s_wait_ready)
    )
  )

  reader.ar.valid := (state === s_idle) && !reset.asBool // Don't send request when resetting
  reader.r.ready  := state === s_wait_mem

  val pc = RegInit(p.ResetVector.S(p.XLEN.W).asUInt)
  pc := Mux(io.in.fire, io.in.bits.dnpc, pc)

  reader.ar.bits.addr := pc

  val inst_reg = RegInit(0.U(32.W))
  inst_reg := Mux(reader.r.fire, reader.r.bits.data, inst_reg)

  io.out.bits.inst := inst_reg
  io.out.bits.pc   := pc

  io.in.ready  := io.out.ready
  io.out.valid := state === s_wait_ready

  val fault_addr = RegInit(0.U(p.XLEN.W))
  fault_addr := Mux(reader.ar.fire, pc, fault_addr)
  val fault_resp = RegInit(AXIResp.OKAY)
  fault_resp := Mux(reader.r.fire, reader.r.bits.resp, fault_resp)

  assert(state =/= s_fault, cf"IFU: Access fault at 0x${fault_addr}%x, resp=${fault_resp}")

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
  PerfCounter(reader.r.fire, "ifu_fetched")
}
