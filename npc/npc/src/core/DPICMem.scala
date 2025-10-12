package core

import chisel3._
import chisel3.util.HasBlackBoxInline

class PMemReadDPICWrapper extends HasBlackBoxInline {
  val io = IO(new Bundle {
    val clock = Input(Clock())
    val en   = Input(Bool())
    val addr = Input(UInt(32.W))
    val out  = Output(UInt(32.W))
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
    val addr         = Input(UInt(32.W))
    val read_enable  = Input(Bool())
    val write_enable = Input(Bool())
    val write_mask   = Input(UInt(8.W))
    val write_data   = Input(UInt(32.W))

    val data_out = Output(UInt(32.W))
    val valid    = Output(Bool())
  })

  val read    = Module(new PMemReadDPICWrapper)
  val read_en = io.read_enable && !reset.asBool

  read.io.clock := clock
  read.io.en   := read_en
  read.io.addr := io.addr
  io.data_out  := read.io.out

  val write    = Module(new PMemWriteDPICWrapper)
  val write_en = io.write_enable && !reset.asBool

  write.io.clock := clock
  write.io.addr  := io.addr
  write.io.en    := write_en
  write.io.data  := io.write_data
  write.io.mask  := io.write_mask

  io.valid := RegNext(read_en)
}
