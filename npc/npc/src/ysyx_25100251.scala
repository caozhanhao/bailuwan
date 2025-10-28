package top

import chisel3._
import core._
import amba._

class ysyx_25100251 extends Module {
  val io = IO(new Bundle {
    val bus       = new AXI4
    val interrupt = Input(Bool())
  })

  implicit val p:        CoreParams  = CoreParams()
  implicit val axi_prop: AXIProperty = AXIProperty()
  val core = Module(new Core)

  core.io.interrupt    := false.B
  core.io.bus.ar.ready := false.B
  core.io.bus.aw.ready := false.B
  core.io.bus.w.ready  := false.B
  core.io.bus.r.valid  := false.B
  core.io.bus.b.valid  := false.B
  core.io.bus.r.bits   := 0.U.asTypeOf(core.io.bus.r.bits)
  core.io.bus.b.bits   := 0.U.asTypeOf(core.io.bus.b.bits)

  io <> core.io
}
