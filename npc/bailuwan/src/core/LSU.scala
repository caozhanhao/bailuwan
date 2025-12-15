// Copyright (c) 2025 caozhanhao
// SPDX-License-Identifier: MIT

package core

import chisel3._
import chisel3.util._
import constants._
import utils.Utils._
import amba._
import utils.{PerfCounter, SignalProbe}
import bailuwan.CoreParams

class LSUOut(
  implicit p: CoreParams)
    extends Bundle {
  val read_data = UInt(p.XLEN.W)
  // Forward from EXU
  val from_exu  = new EXUOutForWBU

  val pc        = UInt(p.XLEN.W)
  val inst      = UInt(32.W)
  val exception = new ExceptionInfo
}

class LSU(
  implicit p: CoreParams,
  axi_prop:   AXIProperty)
    extends Module {
  val io = IO(new Bundle {
    val in  = Flipped(Decoupled(new EXUOut))
    val out = Decoupled(new LSUOut)

    val mem = new AXI4()

    // Hazard
    val hazard = Output(new HazardInfo)

    val wbu_flush = Input(Bool())
  })

  assert(p.XLEN == 32, s"LSU: Unsupported XLEN: ${p.XLEN.toString}");

  val pc        = io.in.bits.pc
  val inst      = io.in.bits.inst
  val prev_excp = io.in.bits.exception

  val req_addr = io.in.bits.lsu.addr
  val req_data = io.in.bits.lsu.store_data
  val wbu_info = io.in.bits.wbu
  val req_op   = Mux(prev_excp.valid, LSUOp.Nop, io.in.bits.lsu.op)

  // States
  val (s_idle :: s_r_addr :: s_r_wait_mem ::
    s_w_addr :: s_w_data :: s_w_wait_mem ::
    s_wait_ready :: Nil) = Enum(7)

  val entry_state = MuxLookup(req_op, s_idle)(
    Seq(
      LSUOp.Nop -> s_wait_ready,
      LSUOp.LB  -> s_r_addr,
      LSUOp.LH  -> s_r_addr,
      LSUOp.LW  -> s_r_addr,
      LSUOp.LBU -> s_r_addr,
      LSUOp.LHU -> s_r_addr,
      LSUOp.SB  -> s_w_addr,
      LSUOp.SH  -> s_w_addr,
      LSUOp.SW  -> s_w_addr
    )
  )

  val state = RegInit(s_idle)
  state := MuxLookup(state, s_idle)(
    Seq(
      // ATTENTION:
      //   1. io.in.valid rather than `fire`.
      //      Because we want the request latched in the pipeline registers during
      //      the transaction.
      //   2. !io.wbu.flush to avoid control hazard.
      //      If WBU is flushing the pipeline, we must NOT enter any state.
      s_idle       -> Mux(io.in.valid && !io.wbu_flush, entry_state, s_idle),
      s_r_addr     -> Mux(io.mem.ar.fire, s_r_wait_mem, s_r_addr),
      s_r_wait_mem -> Mux(io.mem.r.fire, s_wait_ready, s_r_wait_mem),
      s_w_addr     -> Mux(io.mem.aw.fire, Mux(io.mem.w.fire, s_w_wait_mem, s_w_data), s_w_addr),
      s_w_data     -> Mux(io.mem.w.fire, s_w_wait_mem, s_w_data),
      s_w_wait_mem -> Mux(io.mem.b.fire, s_wait_ready, s_w_wait_mem),
      s_wait_ready -> Mux(io.out.fire, s_idle, s_wait_ready)
    )
  )

  io.out.valid := state === s_wait_ready

  // To let the requests latched in the pipeline registers, `io.in.ready` is generally
  // held LOW to "freeze" the requests during execution.
  //
  // `io.in.ready` is asserted HIGH only in two specific circumstances to allow the
  // register to update:
  //
  //   1. Initialization / Bubbles: (state === s_idle && !io.in.valid)
  //        The LSU is idle and the pipeline register contains no data (a bubble).
  //        We assert ready to allow the upstream to place the first (after a bubble) request.
  //   2. Completion: (state === s_wait_ready && io.out.ready)
  //        The current request is finished and accepted by the downstream.
  //        We MUST assert ready to consume the current request in the pipeline registers.
  //        Note that this prevents deadlock. Since `io.in.valid` remains HIGH until
  //        the handshake completes, relying solely on `state === s_idle && !io.in.valid`
  //        to assert ready would cause a deadlock.
  io.in.ready := (state === s_idle && !io.in.valid) || (state === s_wait_ready && io.out.ready)

  // EXU Forward
  io.out.bits.from_exu := wbu_info

  // Read
  io.mem.ar.bits.addr := req_addr
  io.mem.ar.valid     := state === s_r_addr
  io.mem.r.ready      := state === s_r_wait_mem

  val read_reg = Reg(UInt(32.W))
  read_reg := Mux(io.mem.r.fire, io.mem.r.bits.data, read_reg)

  val lb_sel = MuxLookup(req_addr(1, 0), 0.U(8.W))(
    Seq(
      0.U -> read_reg(7, 0),
      1.U -> read_reg(15, 8),
      2.U -> read_reg(23, 16),
      3.U -> read_reg(31, 24)
    )
  )

  val lh_sel = MuxLookup(req_addr(1, 0), 0.U(16.W))(
    Seq(
      0.U -> read_reg(15, 0),
      2.U -> read_reg(31, 16)
    )
  )

  val selected_loaded_data = MuxLookup(req_op, 0.U(p.XLEN.W))(
    Seq(
      LSUOp.LB  -> sign_extend(lb_sel, p.XLEN),
      LSUOp.LH  -> sign_extend(lh_sel, p.XLEN),
      LSUOp.LW  -> read_reg,
      LSUOp.LBU -> zero_extend(lb_sel, p.XLEN),
      LSUOp.LHU -> zero_extend(lh_sel, p.XLEN)
    )
  )

  io.mem.ar.bits.size := MuxLookup(req_op, 0.U(3.W))(
    Seq(
      LSUOp.LB  -> 0.U(3.W),
      LSUOp.LH  -> 1.U(3.W),
      LSUOp.LW  -> 2.U(3.W),
      LSUOp.LBU -> 0.U(3.W),
      LSUOp.LHU -> 1.U(3.W)
    )
  )

  io.out.bits.read_data := selected_loaded_data

  // Store
  val write_mask = MuxLookup(req_op, 0.U(4.W))(
    Seq(
      LSUOp.SB -> (0x1.U(4.W) << req_addr(1, 0)).asUInt,
      LSUOp.SH -> (0x3.U(4.W) << req_addr(1, 0)).asUInt,
      LSUOp.SW -> 0xf.U(4.W)
    )
  )

  val selected_store_data = MuxLookup(req_op, 0.U(p.XLEN.W))(
    Seq(
      LSUOp.SB -> (req_data << (req_addr(1, 0) << 3).asUInt).asUInt,
      LSUOp.SH -> (req_data << (req_addr(1, 0) << 3).asUInt).asUInt,
      LSUOp.SW -> req_data
    )
  )

  io.mem.aw.bits.size := MuxLookup(req_op, 0.U(3.W))(
    Seq(
      LSUOp.SB -> 0.U(3.W),
      LSUOp.SH -> 1.U(3.W),
      LSUOp.SW -> 2.U(3.W)
    )
  )

  io.mem.aw.bits.addr := req_addr
  io.mem.aw.valid     := state === s_w_addr
  io.mem.w.bits.data  := selected_store_data
  io.mem.w.bits.strb  := write_mask
  io.mem.w.valid      := state === s_w_addr || state === s_w_data
  io.mem.b.ready      := state === s_w_wait_mem

  io.mem.ar.bits.id    := 0.U
  io.mem.ar.bits.len   := 0.U // burst length=1, equivalent to an AxLEN value of zero.
  io.mem.ar.bits.burst := 0.U

  io.mem.aw.bits.id    := 0.U
  io.mem.aw.bits.len   := 0.U
  io.mem.aw.bits.burst := 0.U

  io.mem.w.bits.last := true.B

  // Hazard
  io.hazard.valid      := io.in.valid && wbu_info.rd_we
  io.hazard.rd         := wbu_info.rd_addr
  io.hazard.data       := MuxLookup(wbu_info.src_type, 0.U)(
    Seq(
      ExecType.ALU -> wbu_info.alu_out,
      ExecType.CSR -> wbu_info.csr_out,
      ExecType.LSU -> io.out.bits.read_data
    )
  )
  io.hazard.data_valid := wbu_info.src_type =/= ExecType.LSU || state === s_wait_ready

  // Exception
  val excp = Wire(new ExceptionInfo)

  val resp_err = (io.mem.r.fire && io.mem.r.bits.resp =/= AXIResp.OKAY) ||
    (io.mem.b.fire && io.mem.b.bits.resp =/= AXIResp.OKAY)

  val access_fault = RegInit(false.B)
  access_fault := MuxCase(access_fault, Seq((state === s_idle) -> false.B, resp_err -> true.B))

  val is_store = req_op === LSUOp.SB || req_op === LSUOp.SH || req_op === LSUOp.SW
  val cause    = Mux(is_store, ExceptionCode.StoreAMOPageFault, ExceptionCode.LoadAccessFault)

  excp.valid := prev_excp.valid || access_fault
  excp.cause := Mux(prev_excp.valid, prev_excp.cause, cause)
  excp.tval  := Mux(prev_excp.valid, prev_excp.tval, req_addr)

  io.out.bits.pc        := pc
  io.out.bits.inst      := inst
  io.out.bits.exception := excp

  // Debug
  SignalProbe(pc, "lsu_pc")
  SignalProbe(inst, "lsu_inst")

  PerfCounter(io.mem.r.fire, "lsu_read")
  PerfCounter(io.mem.b.fire, "lsu_write")
}
