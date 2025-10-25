package amba

import chisel3._
import chisel3.util._

class AXI4LiteCrossBar(
  val slaves_map: Seq[(Long, Long)]
)(
  implicit p:     AXIProperty)
    extends Module {
  val io = IO(new Bundle {
    val master = Flipped(new AXI4Lite)
    val slaves = Vec(slaves_map.size, new AXI4Lite)
  })
  assert(slaves_map.nonEmpty, "Crossbar for what?")

  val n          = slaves_map.size
  val idx_width  = math.max(1, log2Ceil(n + 1)) // reserve 1 for err
  val real_width = math.max(1, log2Ceil(n))
  val err_idx    = n.U
  val master     = io.master
  val slaves     = io.slaves

  // First block all slaves
  for (i <- 0 until n) {
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
    val (lo, hi) = x._1
    val idx      = x._2.asUInt

    val matched = lo.S.asUInt <= addr && addr < hi.S.asUInt
    matched -> idx
  })

  val dec_err_rbits = Wire(new ReadDataChannel)
  dec_err_rbits.data := 0x25100251.U
  dec_err_rbits.resp := AXIResp.DECERR

  val dec_err_bbits = Wire(new WriteResponseChannel)
  dec_err_bbits.resp := AXIResp.DECERR

  // Read Crossbar
  val r_idle :: r_busy :: Nil = Enum(2)

  val r_state = RegInit(r_idle)

  val r_candidate = MuxCase(err_idx, addr_mux_map(master.ar.bits.addr))

  val r_owner_id = RegInit(0.U(idx_width.W))
  r_owner_id := Mux(r_state === r_idle && master.ar.valid, r_candidate, r_owner_id)
  val r_owner = slaves(r_owner_id(real_width - 1, 0))

  r_state := MuxLookup(r_state, r_idle)(
    Seq(
      r_idle -> Mux(master.ar.valid, r_busy, r_idle),
      r_busy -> Mux(r_owner_id === err_idx || r_owner.r.fire, r_idle, r_busy)
    )
  )

  // r_busy -> addr valid ? connected : decerr
  def if_rbusy[T <: Data](x: T)(err: T = 0.U.asTypeOf(x)) =
    Mux(r_state === r_busy, Mux(r_owner_id =/= err_idx, x, err), 0.U.asTypeOf(x))

  // Connect ar
  r_owner.ar.valid := if_rbusy(true.B)()
  r_owner.ar.bits  := if_rbusy(master.ar.bits)()
  master.ar.ready  := if_rbusy(r_owner.ar.ready)(true.B)

  // printf(cf"Connected: ${r_owner_id}, ar valid: ${r_owner.ar.valid}, ar bits: ${r_owner.ar.bits.addr}\n")

  // Connect r
  master.r.bits   := if_rbusy(r_owner.r.bits)(dec_err_rbits)
  master.r.valid  := if_rbusy(r_owner.r.valid)(true.B)
  r_owner.r.ready := if_rbusy(master.r.ready)()

  // Write Crossbar
  val w_idle :: w_busy :: Nil = Enum(2)

  val w_state = RegInit(w_idle)

  val w_candidate = MuxCase(err_idx, addr_mux_map(master.aw.bits.addr))

  val w_owner_id = RegInit(0.U(idx_width.W))
  w_owner_id := Mux(w_state === w_idle && master.aw.valid, w_candidate, w_owner_id)
  val w_owner = slaves(w_owner_id(real_width - 1, 0))

  w_state := MuxLookup(w_state, w_idle)(
    Seq(
      w_idle -> Mux(master.aw.valid, w_busy, w_idle),
      w_busy -> Mux(w_owner_id === err_idx || w_owner.b.fire, w_idle, w_busy)
    )
  )

  // w_busy -> addr valid ? connected : decerr
  def if_wbusy[T <: Data](x: T)(err: T = 0.U.asTypeOf(x)) =
    Mux(w_state === w_busy, Mux(w_owner_id =/= err_idx, x, err), 0.U.asTypeOf(x))

  // Connect aw
  w_owner.aw.valid := if_wbusy(true.B)()
  w_owner.aw.bits  := if_wbusy(master.aw.bits)()
  master.aw.ready  := if_wbusy(w_owner.aw.ready)(true.B)

  // Connect w
  w_owner.w.valid := if_wbusy(master.w.valid)()
  w_owner.w.bits  := if_wbusy(master.w.bits)()
  master.w.ready  := if_wbusy(w_owner.w.ready)(true.B)

  // Connect b
  master.b.bits   := if_wbusy(w_owner.b.bits)(dec_err_bbits)
  master.b.valid  := if_wbusy(w_owner.b.valid)(true.B)
  w_owner.b.ready := if_wbusy(master.b.ready)()
}
