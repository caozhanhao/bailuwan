package core

import chisel3._
import chisel3.util._
import amba._

class CLINT(
  implicit val axi_prop: AXIProperty)
    extends Module {
  val io = IO(Flipped(new AXI4))

  // AXI4-Lite
  io.r.bits.last := true.B
  io.r.bits.id   := 0.U
  io.b.bits.id   := 0.U

  val mtime = RegInit(0.U(64.W))
  mtime := mtime + 1.U

  val r_idle :: r_wait_ready :: Nil = Enum(2)

  val r_state = RegInit(r_idle)

  r_state := MuxLookup(r_state, r_idle)(
    Seq(
      r_idle       -> Mux(io.ar.fire, r_wait_ready, r_idle),
      r_wait_ready -> Mux(io.r.fire, r_idle, r_wait_ready)
    )
  )

  val addr = RegInit(0.U(axi_prop.ADDR_WIDTH.W))
  addr := Mux(r_state === r_idle, io.ar.bits.addr, addr)

  io.ar.ready    := r_state === r_idle
  io.r.valid     := r_state === r_wait_ready
  io.r.bits.data := Mux(addr === 0xa0000048L.U, mtime(31, 0), mtime(63, 32))
  io.r.bits.resp := AXIResp.OKAY

  io.aw.ready    := true.B
  io.w.ready     := true.B
  io.b.valid     := true.B
  io.b.bits.resp := AXIResp.SLVERR
}
