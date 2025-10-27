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

  io <> core.io
}
