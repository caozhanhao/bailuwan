package core

import chisel3._
import chisel3.util._
import constants._
import top.CoreParams
import utils.Utils._

class LSU(
  implicit p: CoreParams)
    extends Module {
  val io = IO(new Bundle {
    val lsu_op = Input(UInt(LSUOp.WIDTH))
    val addr   = Input(UInt(p.XLEN.W))

    val write_data = Flipped(Decoupled(UInt(p.XLEN.W)))
    val read_data  = Decoupled(UInt(p.XLEN.W))
  })

  assert(p.XLEN == 32, s"LSU: Unsupported XLEN: ${p.XLEN.toString}");

  val mem = Module(new DPICMem())

  // Read

  val read_enable = MuxLookup(io.lsu_op, false.B)(
    Seq(
      LSUOp.LB  -> true.B,
      LSUOp.LH  -> true.B,
      LSUOp.LW  -> true.B,
      LSUOp.LBU -> true.B,
      LSUOp.LHU -> true.B
    )
  )

  val r_idle :: r_wait_mem :: r_wait_ready :: Nil = Enum(3)

  val r_state = RegInit(r_idle)
  r_state := MuxLookup(r_state, r_idle)(
    Seq(
      r_idle       -> Mux(read_enable && mem.io.ar.ready, r_wait_mem, r_idle),
      r_wait_mem   -> Mux(mem.io.r.valid, r_wait_ready, r_wait_mem),
      r_wait_ready -> Mux(io.read_data.ready, r_idle, r_wait_ready)
    )
  )

  mem.io.ar.bits.addr := io.addr
  mem.io.ar.bits.prot := 0.U
  mem.io.ar.valid     := read_enable && r_state === r_idle

  mem.io.r.ready := r_state === r_wait_mem

  val read_reg = Reg(UInt(32.W))
  read_reg := Mux(mem.io.r.valid, mem.io.r.bits.data, read_reg)

  val lb_sel = MuxLookup(io.addr(1, 0), 0.U(8.W))(
    Seq(
      0.U -> read_reg(7, 0),
      1.U -> read_reg(15, 8),
      2.U -> read_reg(23, 16),
      3.U -> read_reg(31, 24)
    )
  )

  val lh_sel = MuxLookup(io.addr(1, 0), 0.U(16.W))(
    Seq(
      0.U -> read_reg(15, 0),
      2.U -> read_reg(31, 16)
    )
  )

  val selected_loaded_data = MuxLookup(io.lsu_op, 0.U(p.XLEN.W))(
    Seq(
      LSUOp.LB  -> sign_extend(lb_sel, p.XLEN),
      LSUOp.LH  -> sign_extend(lh_sel, p.XLEN),
      LSUOp.LW  -> read_reg,
      LSUOp.LBU -> zero_extend(lb_sel, p.XLEN),
      LSUOp.LHU -> zero_extend(lh_sel, p.XLEN)
    )
  )

  io.read_data.valid := r_state === r_wait_ready
  io.read_data.bits  := selected_loaded_data

  // Write

  val write_enable = io.write_data.valid && MuxLookup(io.lsu_op, false.B)(
    Seq(
      LSUOp.SB -> true.B,
      LSUOp.SH -> true.B,
      LSUOp.SW -> true.B
    )
  )

  val write_mask = MuxLookup(io.lsu_op, 0.U(4.W))(
    Seq(
      LSUOp.SB -> (0x1.U(4.W) << io.addr(1, 0)).asUInt,
      LSUOp.SH -> (0x3.U(4.W) << io.addr(1, 0)).asUInt,
      LSUOp.SW -> 0xf.U(4.W)
    )
  )

  val selected_store_data = MuxLookup(io.lsu_op, 0.U(p.XLEN.W))(
    Seq(
      LSUOp.SB -> (io.write_data.bits << (io.addr(1, 0) << 3).asUInt).asUInt,
      LSUOp.SH -> (io.write_data.bits << (io.addr(1, 0) << 3).asUInt).asUInt,
      LSUOp.SW -> io.write_data.bits
    )
  )

  val w_idle :: w_wait_mem :: Nil = Enum(2)

  val w_state = RegInit(w_idle)

  w_state := MuxLookup(w_state, w_idle)(
    Seq(
      w_idle     -> Mux(write_enable && mem.io.aw.ready, w_wait_mem, w_idle),
      w_wait_mem -> Mux(mem.io.w.ready, w_idle, w_wait_mem)
    )
  )

  mem.io.aw.bits.addr := io.addr
  mem.io.aw.bits.prot := 0.U
  mem.io.aw.valid     := write_enable && w_state === w_idle
  mem.io.w.bits.data  := selected_store_data
  mem.io.w.bits.strb  := write_mask
  mem.io.w.valid      := w_state === w_idle
  mem.io.b.ready      := w_state === w_wait_mem
  io.write_data.ready := mem.io.b.ready
}
