package core

import chisel3._
import chisel3.util._
import amba._

class SimConsoleOutput(
  implicit val axi_prop: AXIProperty)
    extends Module {
  val io = IO(Flipped(new AXI4Lite))

  io.ar.ready    := false.B
  io.r.valid     := false.B
  io.r.bits.data := 0.U
  io.r.bits.resp := AXIResp.SLVERR

  val write_enable = io.aw.valid && io.w.valid
  val char         = io.w.bits.data(8, 0)

  io.aw.ready := write_enable
  io.w.ready  := write_enable

  io.b.valid     := RegNext(write_enable, false.B)
  io.b.bits.resp := AXIResp.OKAY

  when(write_enable) {
    printf("%c", char)
  }
}
