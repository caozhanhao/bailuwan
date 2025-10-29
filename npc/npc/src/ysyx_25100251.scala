package top

import chisel3._
import amba._

// External wrapper to set the name
class AXI4MasterTopIO extends Bundle {
  // AW
  val awready = Input(Bool())
  val awvalid = Output(Bool())
  val awaddr  = Output(UInt(32.W))
  val awid    = Output(UInt(4.W))
  val awlen   = Output(UInt(8.W))
  val awsize  = Output(UInt(3.W))
  val awburst = Output(UInt(2.W))

  // W
  val wready = Input(Bool())
  val wvalid = Output(Bool())
  val wdata  = Output(UInt(32.W))
  val wstrb  = Output(UInt(4.W))
  val wlast  = Output(Bool())

  // B
  val bready = Output(Bool())
  val bvalid = Input(Bool())
  val bresp  = Input(UInt(2.W))
  val bid    = Input(UInt(4.W))

  // AR
  val arready = Input(Bool())
  val arvalid = Output(Bool())
  val araddr  = Output(UInt(32.W))
  val arid    = Output(UInt(4.W))
  val arlen   = Output(UInt(8.W))
  val arsize  = Output(UInt(3.W))
  val arburst = Output(UInt(2.W))

  // R
  val rready = Output(Bool())
  val rvalid = Input(Bool())
  val rresp  = Input(UInt(2.W))
  val rdata  = Input(UInt(32.W))
  val rlast  = Input(Bool())
  val rid    = Input(UInt(4.W))
}

class ysyx_25100251 extends Module {
  implicit val p:        CoreParams  = CoreParams()
  implicit val axi_prop: AXIProperty = AXIProperty()

  val io = IO(new Bundle {
    val master    = new AXI4MasterTopIO
    val interrupt = Input(Bool())
  })

  val core = Module(new Core)

  core.io.interrupt := io.interrupt

  // --- AW channel ---
  io.master.awready := core.io.bus.aw.ready
  io.master.awvalid := core.io.bus.aw.valid
  io.master.awaddr  := core.io.bus.aw.bits.addr
  io.master.awid    := core.io.bus.aw.bits.id
  io.master.awlen   := core.io.bus.aw.bits.len
  io.master.awsize  := core.io.bus.aw.bits.size
  io.master.awburst := core.io.bus.aw.bits.burst

  // --- W channel ---
  io.master.wready := core.io.bus.w.ready
  io.master.wvalid := core.io.bus.w.valid
  io.master.wdata  := core.io.bus.w.bits.data
  io.master.wstrb  := core.io.bus.w.bits.strb
  io.master.wlast  := core.io.bus.w.bits.last

  // --- B channel ---
  io.master.bready := core.io.bus.b.ready
  io.master.bvalid := core.io.bus.b.valid
  io.master.bresp  := core.io.bus.b.bits.resp
  io.master.bid    := core.io.bus.b.bits.id

  // --- AR channel ---
  io.master.arready := core.io.bus.ar.ready
  io.master.arvalid := core.io.bus.ar.valid
  io.master.araddr  := core.io.bus.ar.bits.addr
  io.master.arid    := core.io.bus.ar.bits.id
  io.master.arlen   := core.io.bus.ar.bits.len
  io.master.arsize  := core.io.bus.ar.bits.size
  io.master.arburst := core.io.bus.ar.bits.burst

  // --- R channel ---
  io.master.rready := core.io.bus.r.ready
  io.master.rvalid := core.io.bus.r.valid
  io.master.rresp  := core.io.bus.r.bits.resp
  io.master.rdata  := core.io.bus.r.bits.data
  io.master.rlast  := core.io.bus.r.bits.last
  io.master.rid    := core.io.bus.r.bits.id
}
