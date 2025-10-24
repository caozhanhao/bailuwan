package amba

import chisel3._
import chisel3.util._

class AXI4LiteArbiter(
  val n:      Int
)(
  implicit p: AXIProperty)
    extends Module {
  val io = IO(new Bundle {
    val masters = Vec(n, Flipped(new AXI4Lite))
    val slave   = new AXI4Lite
  })

  assert(n > 1, "Arbiter what?")

  val idx_width = math.max(1, log2Ceil(n))
  val masters   = io.masters
  val slave     = io.slave

  // First block all masters
  for (i <- 0 until n) {
    val m = masters(i)
    m.ar.ready := false.B
    m.aw.ready := false.B
    m.w.ready  := false.B

    m.r.valid := false.B
    m.r.bits  := 0.U.asTypeOf(m.r.bits)

    m.b.valid := false.B
    m.b.bits  := 0.U.asTypeOf(m.b.bits)
  }

  // Read Arbiter
  val r_idle :: r_busy :: Nil = Enum(2)

  val r_state     = RegInit(r_idle)
  val r_requests  = VecInit(masters.map(_.ar.valid))
  val r_candidate = PriorityEncoder(r_requests)
  val any_read    = r_requests.asUInt.orR
  val r_owner_id  = RegInit(0.U(idx_width.W))
  r_owner_id := Mux(r_state === r_idle, r_candidate, r_owner_id)
  val r_owner = masters(r_owner_id)

  r_state := MuxLookup(r_state, r_idle)(
    Seq(
      r_idle -> Mux(any_read, r_busy, r_idle),
      r_busy -> Mux(r_owner.r.ready, r_idle, r_busy)
    )
  )

  def if_rbusy[T <: Data](x: T) = Mux(r_state === r_busy, x, 0.U.asTypeOf(x))

  // Connect ar
  slave.ar.valid   := if_rbusy(true.B)
  slave.ar.bits    := if_rbusy(r_owner.ar.bits)
  r_owner.ar.ready := if_rbusy(slave.ar.ready)

  // Connect r
  r_owner.r.bits  := if_rbusy(slave.r.bits)
  r_owner.r.valid := if_rbusy(slave.r.valid)
  slave.r.ready   := if_rbusy(r_owner.r.ready)

  // Write Arbiter
  val w_idle :: w_busy :: Nil = Enum(2)

  val w_state     = RegInit(w_idle)
  val w_requests  = VecInit(masters.map(_.aw.valid))
  val w_candidate = PriorityEncoder(w_requests)
  val any_write   = w_requests.asUInt.orR
  val w_owner_id  = RegInit(0.U(idx_width.W))
  w_owner_id := Mux(w_state === w_idle, w_candidate, w_owner_id)
  val w_owner = masters(w_owner_id)

  w_state := MuxLookup(w_state, w_idle)(
    Seq(
      w_idle -> Mux(any_write, w_busy, w_idle),
      w_busy -> Mux(w_owner.b.ready, w_idle, w_busy)
    )
  )

  def if_wbusy[T <: Data](x: T) = Mux(w_state === w_busy, x, 0.U.asTypeOf(x))

  // Connect aw
  slave.aw.valid   := if_wbusy(true.B)
  slave.aw.bits    := if_wbusy(w_owner.aw.bits)
  w_owner.aw.ready := if_wbusy(slave.aw.ready)

  // Connect w
  slave.w.bits    := if_wbusy(w_owner.w.bits)
  slave.w.valid   := if_wbusy(w_owner.w.valid)
  w_owner.w.ready := if_wbusy(slave.w.ready)

  // Connect b
  w_owner.b.bits  := if_wbusy(slave.b.bits)
  w_owner.b.valid := if_wbusy(slave.b.valid)
  slave.b.ready   := if_wbusy(w_owner.b.ready)
}
