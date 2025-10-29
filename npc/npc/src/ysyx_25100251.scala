package top

import chisel3._
import amba._

class ysyx_25100251 extends Module {
  implicit val p:        CoreParams  = CoreParams()
  implicit val axi_prop: AXIProperty = AXIProperty()

  val io = IO(new Bundle {
    val bus       = new AXI4
    val interrupt = Input(Bool())
  })

  val core = Module(new Core)

  core.io.interrupt := io.interrupt
  core.io.bus <> io.bus
}
