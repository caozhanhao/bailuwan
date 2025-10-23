package core

import chisel3._
import chisel3.util._

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
      |      out = pmem_read(addr);
      |    else
      |      out = 0;
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
  val io = IO(new Bundle {
    val req_valid    = Input(Bool())
    val addr         = Input(UInt(32.W))
    val read_enable  = Input(Bool())
    val write_enable = Input(Bool())
    val write_mask   = Input(UInt(8.W))
    val write_data   = Input(UInt(32.W))

    val data_out    = Output(UInt(32.W))
    val read_valid  = Output(Bool())
    val write_ready = Output(Bool())
  })
  val read    = Module(new PMemReadDPICWrapper)
  val read_en = io.req_valid && io.read_enable && !reset.asBool

  val read_reg = RegInit(0.U(32.W))
  read_reg := Mux(read_en, read.io.out, read_reg)

  read.io.clock := clock
  read.io.en    := read_en
  read.io.addr  := io.addr
  io.data_out   := read_reg

  val lfsr = random.FibonacciLFSR.maxPeriod(32).asBool
  io.read_valid  := RegNext(io.req_valid, false.B) && lfsr

  val write    = Module(new PMemWriteDPICWrapper)
  val write_en = io.req_valid && io.write_enable && !reset.asBool

  write.io.clock := clock
  write.io.addr  := io.addr
  write.io.en    := write_en
  write.io.data  := io.write_data
  write.io.mask  := io.write_mask

  io.write_ready := true.B
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
