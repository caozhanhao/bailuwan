package core

import chisel3._
import chisel3.util._
import amba._

class PMemReadDPICWrapper extends HasBlackBoxInline {
  val io = IO(new Bundle {
    val clock = Input(Clock())
    val en    = Input(Bool())
    val addr  = Input(UInt(32.W))
    val out   = Output(UInt(32.W))
  })
  setInline(
    "PMemReadDPICWrapper.sv",
    """
      |module PMemReadDPICWrapper(
      |  input clock,
      |  input en,
      |  input int addr,
      |  output int out
      |);
      |  import "DPI-C" function int pmem_read(input int addr);
      |  always @(posedge clock) begin
      |    if (en)
      |      out <= pmem_read(addr);
      |    else
      |      out <= 0;
      |  end
      |endmodule
      |""".stripMargin
  )
}

class PMemWriteDPICWrapper extends HasBlackBoxInline {
  val io = IO(new Bundle {
    val clock = Input(Clock())
    val en    = Input(Bool())
    val addr  = Input(UInt(32.W))
    val data  = Input(UInt(32.W))
    val mask  = Input(UInt(8.W))
  })
  setInline(
    "PMemWriteDPICWrapper.sv",
    """
      |module PMemWriteDPICWrapper(
      |  input clock,
      |  input en,
      |  input int addr,
      |  input int data,
      |  input byte mask
      |);
      |  import "DPI-C" function void pmem_write(input int addr, input int data, input byte mask);
      |  always @(posedge clock) begin
      |    if (en)
      |      pmem_write(addr, data, mask);
      |  end
      |endmodule
      |""".stripMargin
  )
}

class DPICMem extends Module {
  implicit val axiprop: AXIProperty = AXIProperty()
  val io = IO(Flipped(new AXI4Lite))

  // Read
  val mem_read = Module(new PMemReadDPICWrapper)
  mem_read.io.clock := clock

  val r_idle :: r_wait_mem :: r_wait_ready :: Nil = Enum(3)

  val r_state = RegInit(r_idle)

  mem_read.io.addr := io.ar.bits.addr
  mem_read.io.en   := io.ar.fire && r_state === r_idle && !reset.asBool

  val read_data_reg = RegInit(0.U(32.W))
  read_data_reg := Mux(r_state === r_wait_mem, mem_read.io.out, read_data_reg)

  io.r.bits.data := read_data_reg
  io.r.bits.resp := AXIResp.OKAY
  // io.r.valid     := utils.RandomDelay(r_state === r_wait_ready)
  io.r.valid     := r_state === r_wait_ready
  io.ar.ready    := r_state === r_idle

  r_state := MuxLookup(r_state, r_idle)(
    Seq(
      r_idle       -> Mux(mem_read.io.en, r_wait_mem, r_idle),
      r_wait_mem   -> r_wait_ready, // read to register takes one cycle
      r_wait_ready -> Mux(io.r.ready, r_idle, r_wait_ready)
    )
  )

  // Write
  val mem_write = Module(new PMemWriteDPICWrapper)
  mem_write.io.clock := clock

  val w_idle :: w_wait_ready :: Nil = Enum(2)

  val w_state = RegInit(w_idle)
  mem_write.io.addr := io.aw.bits.addr
  mem_write.io.en   := io.aw.fire && io.w.fire && w_state === w_idle && !reset.asBool
  mem_write.io.data := io.w.bits.data
  mem_write.io.mask := io.w.bits.strb
  io.w.ready        := w_state === w_idle
  io.aw.ready       := w_state === w_idle

  io.b.valid     := utils.RandomDelay(w_state === w_wait_ready)
  // io.b.valid     := w_state === w_wait_ready
  io.b.bits.resp := AXIResp.OKAY

  w_state := MuxLookup(w_state, w_idle)(
    Seq(
      w_idle       -> Mux(mem_write.io.en, w_wait_ready, w_idle),
      w_wait_ready -> Mux(io.b.ready, w_idle, w_wait_ready)
    )
  )
}

//class TempMemForSTA extends Module {
//  val io = IO(new Bundle {
//    val addr         = Input(UInt(32.W))
//    val read_enable  = Input(Bool())
//    val write_enable = Input(Bool())
//    val write_mask   = Input(UInt(8.W))
//    val write_data   = Input(UInt(32.W))
//
//    val data_out = Output(UInt(32.W))
//    val valid    = Output(Bool())
//  })
//
//  val mem = RegInit(VecInit(Seq.fill(256)(0.U(32.W))))
//
//  val addr = io.addr >> 2
//
//  def apply_mask = (original_data: UInt, data: UInt, mask: UInt) => {
//    val ori0 = original_data(7, 0)
//    val ori1 = original_data(15, 8)
//    val ori2 = original_data(23, 16)
//    val ori3 = original_data(31, 24)
//
//    val new0 = data(7, 0)
//    val new1 = data(15, 8)
//    val new2 = data(23, 16)
//    val new3 = data(31, 24)
//
//    Mux(mask(0), new0, ori0) ## Mux(mask(1), new1, ori1) ## Mux(mask(2), new2, ori2) ## Mux(mask(3), new3, ori3)
//  }
//
//  val masked = apply_mask(mem(addr(7, 0)), io.write_data, io.write_mask)
//  mem(addr(7, 0)) := Mux(io.write_enable, masked, mem(addr(7, 0)))
//
//  io.data_out := Mux(io.read_enable, mem(addr(7, 0)), 0.U)
//  io.valid    := io.read_enable
//}
