package core

import chisel3._
import chisel3.util.HasBlackBoxInline

class PMemReadDPICWrapper extends HasBlackBoxInline {
  val io = IO(new Bundle {
    val en   = Input(Bool())
    val addr = Input(UInt(32.W))
    val out  = Output(UInt(32.W))
  })
  setInline(
    "PMemReadDPICWrapper.sv",
    """
      |module PMemReadDPICWrapper(
      |  input en,
      |  input int addr,
      |  output int out
      |);
      |  import "DPI-C" function int pmem_read(input int addr);
      |  always @(*) begin
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
    val en   = Input(Bool())
    val addr = Input(UInt(32.W))
    val data = Input(UInt(32.W))
    val mask = Input(UInt(8.W))
  })
  setInline(
    "PMemWriteDPICWrapper.sv",
    """
      |module PMemWriteDPICWrapper(
      |  input en,
      |  input int addr,
      |  input int data,
      |  input byte mask
      |);
      |  import "DPI-C" function void pmem_write(input int addr, input int data, input byte mask);
      |  always @(posedge clk) begin
      |    if (en)
      |      pmem_write(addr, data, mask);
      |  end
      |endmodule
      |""".stripMargin
  )
}

class DPICMem extends Module {
  val io = IO(new Bundle {
    val addr = Input(UInt(32.W))
    val read_enable = Input(Bool())
    val write_enable = Input(Bool())
    val write_mask = Input(UInt(8.W))
    val write_data = Input(UInt(32.W))

    val data_out = Output(UInt(32.W))
    val valid = Output(Bool())
  })

  val read = Module(new PMemReadDPICWrapper)
  read.io.addr := io.addr
  read.io.en   := io.read_enable && !reset.asBool
  io.data_out  := read.io.out

  val write = Module(new PMemWriteDPICWrapper)
  write.io.addr := io.addr
  write.io.en   := io.write_enable && !reset.asBool
  write.io.data := io.write_data
  write.io.mask := io.write_mask

  io.valid := read.io.en
}
