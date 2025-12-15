// Copyright (c) 2025 caozhanhao
// SPDX-License-Identifier: MIT

package core

import chisel3._
import chisel3.util._
import amba._
import utils.{PerfCounter, SignalProbe}
import bailuwan.CoreParams
import constants.ExceptionCode

class IFUOut(
  implicit p: CoreParams)
    extends Bundle {
  val pc        = UInt(p.XLEN.W)
  val inst      = UInt(32.W)
  val exception = new ExceptionInfo
}

class IFU(
  implicit p: CoreParams,
  axi_prop:   AXIProperty)
    extends Module {
  val io = IO(new Bundle {
    val out = Decoupled(new IFUOut)

    val redirect_valid  = Input(Bool())
    val redirect_target = Input(UInt(p.XLEN.W))

    val mem          = new AXI4()
    val icache_flush = Input(Bool())
  })

  val icache    = Module(new ICache())
  val icache_io = icache.io.ifu

  icache.io.mem <> io.mem
  icache.io.flush := io.icache_flush

  val pc = RegInit(p.ResetVector.S(p.XLEN.W).asUInt)

  val dnpc = MuxCase(
    pc,
    Seq(
      io.redirect_valid  -> io.redirect_target,
      icache_io.req.fire -> (pc + 4.U)
    )
  )
  pc := dnpc

  val resp_queue = Module(new Queue(new IFUOut, entries = 4, hasFlush = true))

  // Exception
  val is_misaligned   = icache_io.resp.bits.addr(1, 0) =/= 0.U
  val is_access_fault = icache_io.resp.bits.error
  val excp            = Wire(new ExceptionInfo)
  excp.valid := is_misaligned || is_access_fault
  excp.cause := Mux(
    is_misaligned,
    ExceptionCode.InstructionAddressMisaligned,
    ExceptionCode.InstructionAccessFault
  )
  excp.tval  := icache_io.resp.bits.addr

  // IO
  icache_io.kill          := io.redirect_valid
  icache_io.req.bits.addr := pc
  icache_io.req.valid     := !reset.asBool && !io.redirect_valid
  icache_io.resp.ready    := resp_queue.io.enq.ready

  resp_queue.io.enq.valid          := icache_io.resp.valid
  resp_queue.io.enq.bits.pc        := icache_io.resp.bits.addr
  resp_queue.io.enq.bits.inst      := icache_io.resp.bits.data
  resp_queue.io.enq.bits.exception := excp
  resp_queue.io.flush.get          := io.redirect_valid

  io.out.bits             := resp_queue.io.deq.bits
  io.out.valid            := resp_queue.io.deq.valid && !io.redirect_valid
  resp_queue.io.deq.ready := io.out.ready && !io.redirect_valid

  assert(
    !icache_io.resp.valid || !icache_io.resp.bits.error,
    cf"IFU: Access fault at 0x${RegEnable(pc, icache_io.req.fire)}%x"
  )

  SignalProbe(pc, "ifu_pc")
  PerfCounter(icache_io.resp.fire, "ifu_fetched")
}
