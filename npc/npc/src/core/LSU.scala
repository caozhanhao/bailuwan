package core

import chisel3._
import chisel3.util._
import constants._
import utils.Utils._

class LSU extends Module {
  val io = IO(new Bundle {
    val lsu_op     = Input(UInt(LSUOp.WIDTH))
    val addr       = Input(UInt(32.W))
    val write_data = Input(UInt(32.W))
    val read_data  = Output(UInt(32.W))
  })

  val mem = Module(new DPICMem())

  val write_enable = MuxLookup(io.lsu_op, false.B)(
    Seq(
      LSUOp.SB -> true.B,
      LSUOp.SH -> true.B,
      LSUOp.SW -> true.B
    )
  )
  val read_enable  = io.lsu_op =/= LSUOp.None && !write_enable

  val write_enable_pulse = write_enable && !RegNext(write_enable, false.B)
  val read_enable_pulse = read_enable && !RegNext(read_enable, false.B)

  val write_mask = MuxLookup(io.lsu_op, 0.U(8.W))(
    Seq(
      LSUOp.SB -> (0x1.U(8.W) << io.addr(1, 0)).asUInt,
      LSUOp.SH -> (0x3.U(8.W) << io.addr(1, 0)).asUInt,
      LSUOp.SW -> 0xf.U(8.W)
    )
  )

  val selected_store_data = MuxLookup(io.lsu_op, 0.U(32.W))(
    Seq(
      LSUOp.SB -> (io.write_data << (io.addr(1, 0) << 3).asUInt).asUInt,
      LSUOp.SH -> (io.write_data << (io.addr(1, 0) << 3).asUInt).asUInt,
      LSUOp.SW -> io.write_data
    )
  )

  val data_out = mem.io.data_out

  val lb_sel = MuxLookup(io.addr(1, 0), 0.U(8.W))(
    Seq(
      0.U -> data_out(7, 0),
      1.U -> data_out(15, 8),
      2.U -> data_out(23, 16),
      3.U -> data_out(31, 24)
    )
  )

  val lh_sel = MuxLookup(io.addr(1, 0), 0.U(16.W))(
    Seq(
      0.U -> data_out(15, 0),
      2.U -> data_out(31, 16)
    )
  )

  val selected_loaded_data = MuxLookup(io.lsu_op, 0.U(32.W))(
    Seq(
      LSUOp.LB  -> sign_extend(lb_sel, 32),
      LSUOp.LH  -> sign_extend(lh_sel, 32),
      LSUOp.LW  -> data_out,
      LSUOp.LBU -> zero_extend(lb_sel, 32),
      LSUOp.LHU -> zero_extend(lh_sel, 32)
    )
  )

  mem.io.addr         := io.addr
  mem.io.read_enable  := read_enable_pulse
  mem.io.write_enable := write_enable_pulse
  mem.io.write_mask   := write_mask
  mem.io.write_data   := selected_store_data
  io.read_data        := selected_loaded_data
}
