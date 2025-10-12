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
      |  always @(*) begin
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

  val read_rising = io.read_enable && !RegNext(io.read_enable, false.B)
  val read_en = read_rising && !reset.asBool

  val read_reg = RegInit(0.U(32.W))
  read_reg := read.io.out
  read.io.en := read_en
  read.io.addr := io.addr
  io.data_out  := read_reg

  val write = Module(new PMemWriteDPICWrapper)

  val write_rising = io.write_enable && !RegNext(io.write_enable, false.B)
  val write_en = write_rising && !reset.asBool

  val write_reg = RegInit(0.U(32.W))
  write_reg := io.write_data

  write.io.addr := io.addr
  write.io.en   := write_en
  write.io.data := write_reg
  write.io.mask := io.write_mask

  io.valid := RegNext(read_en)
}
