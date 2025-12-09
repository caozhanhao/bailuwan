// Copyright (c) 2025 caozhanhao
// SPDX-License-Identifier: MIT

package utils

import amba._
import chisel3._
import chisel3.util._

class PMemReadDPICWrapper extends HasBlackBoxInline {
  val io = IO(new Bundle {
    val clock = Input(Clock())
    val en    = Input(Bool())
    val addr  = Input(UInt(32.W))
    val out   = Output(UInt(32.W))
  })
  setInline(
    "PMemReadDPICWrapper.sv",
    """
      |module PMemReadDPICWrapper(
      |  input clock,
      |  input en,
      |  input int addr,
      |  output int out
      |);
      |  import "DPI-C" function int pmem_read(input int addr);
      |  always @(posedge clock) begin
      |    if (en)
      |      out <= pmem_read(addr);
      |    else
      |      out <= 0;
      |  end
      |endmodule
      |""".stripMargin
  )
}

class PMemWriteDPICWrapper extends HasBlackBoxInline {
  val io = IO(new Bundle {
    val clock = Input(Clock())
    val en    = Input(Bool())
    val addr  = Input(UInt(32.W))
    val data  = Input(UInt(32.W))
    val mask  = Input(UInt(8.W))
  })
  setInline(
    "PMemWriteDPICWrapper.sv",
    """
      |module PMemWriteDPICWrapper(
      |  input clock,
      |  input en,
      |  input int addr,
      |  input int data,
      |  input byte mask
      |);
      |  import "DPI-C" function void pmem_write(input int addr, input int data, input byte mask);
      |  always @(posedge clock) begin
      |    if (en)
      |      pmem_write(addr, data, mask);
      |  end
      |endmodule
      |""".stripMargin
  )
}

class DPICMem(
  implicit axi_prop: AXIProperty)
    extends Module {
  val io = IO(Flipped(new AXI4))

  // FIXME
  io.r.bits.id := 0.U
  io.b.bits.id := 0.U

  def compute_next_addr(curr_addr: UInt, size: UInt, burst: UInt): UInt = {
    val incr = (1.U << size).asUInt
    Mux(burst === AXIBurstType.FIXED, curr_addr, curr_addr + incr)
  }

  // Read
  val mem_read = Module(new PMemReadDPICWrapper)
  mem_read.io.clock := clock

  val r_idle :: r_reading :: Nil = Enum(2)

  val r_state = RegInit(r_idle)
  val r_ctx   = RegEnable(io.ar.bits, io.ar.fire)

  val r_addr = RegInit(0.U(32.W))
  val r_cnt  = RegInit(0.U(8.W))

  val next_addr =
    Mux(io.ar.fire, io.ar.bits.addr, Mux(io.r.fire, compute_next_addr(r_addr, r_ctx.size, r_ctx.burst), r_addr))
  r_addr := next_addr

  r_cnt := Mux(io.ar.fire, 0.U, Mux(io.r.fire, r_cnt + 1.U, r_cnt))

  io.r.bits.last := r_cnt === r_ctx.len

  mem_read.io.addr := next_addr
  mem_read.io.en   := ((io.ar.fire && r_state === r_idle) || (r_state === r_reading)) && !reset.asBool

  io.r.bits.data := mem_read.io.out
  io.r.bits.resp := AXIResp.OKAY
  io.r.valid     := r_state === r_reading
  io.ar.ready    := r_state === r_idle

  r_state := MuxLookup(r_state, r_idle)(
    Seq(
      r_idle    -> Mux(io.ar.fire, r_reading, r_idle),
      r_reading -> Mux(io.r.fire && io.r.bits.last, r_idle, r_reading)
    )
  )

  // Write
  val mem_write = Module(new PMemWriteDPICWrapper)
  mem_write.io.clock := clock

  val w_idle :: w_wait_ready :: Nil = Enum(2)

  val w_state = RegInit(w_idle)
  mem_write.io.addr := io.aw.bits.addr
  mem_write.io.en   := io.aw.fire && io.w.fire && w_state === w_idle && !reset.asBool
  mem_write.io.data := io.w.bits.data
  mem_write.io.mask := io.w.bits.strb
  io.w.ready        := w_state === w_idle
  io.aw.ready       := w_state === w_idle

  // io.b.valid     := utils.RandomDelay(w_state === w_wait_ready)
  io.b.valid     := w_state === w_wait_ready
  io.b.bits.resp := AXIResp.OKAY

  w_state := MuxLookup(w_state, w_idle)(
    Seq(
      w_idle       -> Mux(mem_write.io.en, w_wait_ready, w_idle),
      w_wait_ready -> Mux(io.b.fire, w_idle, w_wait_ready)
    )
  )
}
