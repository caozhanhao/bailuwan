package amba

import chisel3._
import chisel3.util._

class AXI4CrossBar(
  val slaves_map: Seq[Seq[(Long, Long)]]
)(
  implicit p:     AXIProperty)
    extends Module {
  val io = IO(new Bundle {
    val master = Flipped(new AXI4)
    val slaves = Vec(slaves_map.size, new AXI4)
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
    val ranges = x._1
    val idx    = x._2.asUInt

    val matches = VecInit(ranges.map(rng => {
      val (lo, hi) = rng
      lo.S.asUInt <= addr && addr < hi.S.asUInt
    })).asUInt

    matches.orR -> idx
  })

  val dec_err = Wire(new AXI4())
  dec_err             := 0.U.asTypeOf(dec_err)
  dec_err.r.bits.data := 0x25100251.U
  dec_err.r.bits.resp := AXIResp.DECERR
  dec_err.r.valid     := true.B
  dec_err.b.bits.resp := AXIResp.DECERR
  dec_err.b.valid     := true.B

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
  def if_rbusy[T <: Data](x: T, err: T) =
    Mux(r_state === r_busy, Mux(r_owner_id =/= err_idx, x, err), 0.U.asTypeOf(x))

  // Connection
  master.ar <> if_rbusy(r_owner.ar, dec_err.ar)
  master.r <> if_rbusy(r_owner.r, dec_err.r)

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
  def if_wbusy[T <: Data](x: T, err: T) =
    Mux(w_state === w_busy, Mux(w_owner_id =/= err_idx, x, err), 0.U.asTypeOf(x))

  // Connection
  master.aw <> if_wbusy(w_owner.aw, dec_err.aw)
  master.w <> if_wbusy(w_owner.w, dec_err.w)
  master.b <> if_wbusy(w_owner.b, dec_err.b)
}
