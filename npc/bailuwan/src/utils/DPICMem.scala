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

  def compute_next_addr(curr_addr: UInt, size: UInt, burst: UInt): UInt = {
    val incr = (1.U << size).asUInt
    Mux(burst === AXIBurstType.FIXED, curr_addr, curr_addr + incr)
  }

  // Read
  val mem_read = Module(new PMemReadDPICWrapper)
  mem_read.io.clock := clock

  val r_idle :: r_data_valid :: Nil = Enum(2)

  val r_state = RegInit(r_idle)
  val r_ctx   = RegEnable(io.ar.bits, io.ar.fire)

  val r_addr = RegInit(0.U(32.W))
  val r_cnt  = RegInit(0.U(8.W))

  val r_next_addr =
    Mux(io.ar.fire, io.ar.bits.addr, Mux(io.r.fire, compute_next_addr(r_addr, r_ctx.size, r_ctx.burst), r_addr))
  r_addr := r_next_addr

  r_cnt := Mux(io.ar.fire, 0.U, Mux(io.r.fire, r_cnt + 1.U, r_cnt))

  io.r.bits.last := r_cnt === r_ctx.len

  // ATTENTION: r_next_addr here.
  mem_read.io.addr := r_next_addr
  mem_read.io.en   := ((io.ar.fire && r_state === r_idle) ||
    (r_state === r_data_valid && !(io.r.fire && io.r.bits.last))) && !reset.asBool

  io.r.bits.data := mem_read.io.out
  io.r.bits.resp := AXIResp.OKAY
  io.r.valid     := r_state === r_data_valid
  io.ar.ready    := r_state === r_idle
  io.r.bits.id   := r_ctx.id

  r_state := MuxLookup(r_state, r_idle)(
    Seq(
      r_idle       -> Mux(io.ar.fire, r_data_valid, r_idle),
      r_data_valid -> Mux(io.r.fire && io.r.bits.last, r_idle, r_data_valid)
    )
  )

  // Write
  val mem_write = Module(new PMemWriteDPICWrapper)
  mem_write.io.clock := clock

  val w_idle :: w_data :: w_resp :: Nil = Enum(3)

  val w_state = RegInit(w_idle)
  val w_ctx   = RegEnable(io.aw.bits, io.aw.fire)
  val w_addr  = RegInit(0.U(32.W))

  val w_next_addr =
    Mux(io.aw.fire, io.aw.bits.addr, Mux(io.w.fire, compute_next_addr(w_addr, w_ctx.size, w_ctx.burst), w_addr))

  w_addr := w_next_addr

  // ATTENTION: w_addr here.
  mem_write.io.addr := w_addr
  mem_write.io.data := io.w.bits.data
  mem_write.io.mask := io.w.bits.strb
  mem_write.io.en   := (w_state === w_data) && io.w.fire && !reset.asBool

  io.aw.ready    := w_state === w_idle
  io.w.ready     := w_state === w_data
  io.b.valid     := w_state === w_resp
  io.b.bits.resp := AXIResp.OKAY
  io.b.bits.id   := w_ctx.id

  w_state := MuxLookup(w_state, w_idle)(
    Seq(
      w_idle -> Mux(io.aw.fire, w_data, w_idle),
      w_data -> Mux(io.w.fire && io.w.bits.last, w_resp, w_data),
      w_resp -> Mux(io.b.fire, w_idle, w_resp)
    )
  )
}
