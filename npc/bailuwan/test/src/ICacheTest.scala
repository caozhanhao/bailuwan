// Copyright (c) 2025-2026 caozhanhao
// SPDX-License-Identifier: MulanPSL-2.0

package bailuwan

import core._
import amba._

import chisel3._
import chisel3.util._
import chiseltest._
import chiseltest.formal._
import org.scalatest.flatspec.AnyFlatSpec

// TODO: AXI-Burst
class ICacheTestTop extends Module {
  // `Debug = false` to disable PerfCounters
  implicit val p:        CoreParams  = CoreParams(Debug = false)
  implicit val axi_prop: AXIProperty = AXIProperty()

  val io = IO(new Bundle {
    val req_valid  = Input(Bool())
    val req_addr   = Input(UInt(p.XLEN.W))
    val resp_ready = Input(Bool())

    val block_ar = Input(Bool())
    val block_r  = Input(Bool())
  })

  // DUT
  val dut = Module(new ICache)

  dut.io.ifu.req.valid     := io.req_valid
  dut.io.ifu.req.bits.addr := io.req_addr
  dut.io.ifu.resp.ready    := io.resp_ready

  // Ensure request is sent only when dut is ready
  when(!dut.io.ifu.req.ready) {
    assume(!io.req_valid)
  }

  // Ref
  val MEM_SIZE = 128
  val mem      = Mem(MEM_SIZE / 4, UInt(32.W))

  // Addr
  dut.io.mem.ar.ready := !io.block_ar

  val ar_pending  = RegInit(false.B)
  val ar_addr_reg = Reg(UInt(32.W))

  when(dut.io.mem.ar.fire) {
    ar_pending  := true.B
    ar_addr_reg := dut.io.mem.ar.bits.addr
  }

  val mem_rdata = mem(ar_addr_reg(31, 2))

  // R valid only when we have a pending request
  dut.io.mem.r.valid     := ar_pending && !io.block_r
  dut.io.mem.r.bits.data := mem_rdata
  dut.io.mem.r.bits.resp := AXIResp.OKAY
  dut.io.mem.r.bits.last := true.B
  dut.io.mem.r.bits.id   := 0.U

  // Clear ar_pending when R fire
  when(dut.io.mem.r.fire) {
    ar_pending := false.B
  }

  dut.io.mem.aw.ready := false.B
  dut.io.mem.w.ready  := false.B
  dut.io.mem.b.valid  := false.B
  dut.io.mem.b.bits   := DontCare

  // Verify
  val last_req_addr = Reg(UInt(p.XLEN.W))
  when(dut.io.ifu.req.fire) {
    last_req_addr := io.req_addr
  }

  // Calculate the Expected Data:
  // If `req.fire` is happening NOW, it might be a combinational HIT, so check `io.req_addr`.
  // Otherwise, check `last_req_addr`.
  val check_addr    = Mux(dut.io.ifu.req.fire, io.req_addr, last_req_addr)
  val expected_data = mem(check_addr(31, 2))

  when(dut.io.ifu.resp.valid) {
    assert(dut.io.ifu.resp.bits.data === expected_data, "Cache data mismatch with Ref Mem")
    printf("1 Transaction Done\n")
  }
}

class ICacheTest extends AnyFlatSpec with ChiselScalatestTester with Formal {
  "ICache" should "pass" in {
    verify(new ICacheTestTop, Seq(BoundedCheck(10), BtormcEngineAnnotation))
  }
}
