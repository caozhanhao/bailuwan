// Copyright (c) 2025 caozhanhao
// SPDX-License-Identifier: MIT

package core

import chisel3.{Bundle, _}
import chisel3.util._
import constants._
import amba._
import utils.{PerfCounter, SignalProbe}
import bailuwan.CoreParams

class EXUOutForWBU(
  implicit p: CoreParams)
    extends Bundle {
  val src_type = UInt(ExecType.WIDTH)
  val rd_addr  = UInt(5.W)
  val rd_we    = Bool()

  // ALU
  val alu_out     = UInt(p.XLEN.W)
  // CSR Out:
  //   CSR{RW, RS, RC}[I] -> the CSR indicated by inst[31:20]
  //   ECall              -> mtvec
  //   MRet               -> mepc
  val csr_out     = UInt(p.XLEN.W)
  val csr_rd_addr = UInt(12.W)
  val csr_rd_data = UInt(p.XLEN.W)

  val is_trap_return = Bool()
}

class EXUOutForLSU(
  implicit p: CoreParams)
    extends Bundle {
  val op         = UInt(LSUOp.WIDTH)
  val addr       = UInt(p.XLEN.W)
  val store_data = UInt(p.XLEN.W)
}

class EXUOut(
  implicit p: CoreParams)
    extends Bundle {
  val lsu = new EXUOutForLSU
  val wbu = new EXUOutForWBU

  val pc        = UInt(p.XLEN.W)
  val inst      = UInt(32.W)
  val exception = new ExceptionInfo
}

class EXU(
  implicit p: CoreParams)
    extends Module {
  val io = IO(new Bundle {
    val in  = Flipped(Decoupled(new IDUOut))
    val out = Decoupled(new EXUOut)

    // CSRFile
    val csr_rs_addr = Output(UInt(12.W))
    val csr_rs_en   = Output(Bool())
    val csr_rs_data = Input(UInt(p.XLEN.W))

    // Branch
    val br_valid  = Output(Bool())
    val br_target = Output(UInt(p.XLEN.W))

    // ICache
    val icache_flush = Output(Bool())

    // Hazard
    val hazard = Output(new HazardInfo)
  })

  val decoded = io.in.bits

  val exec_type = decoded.exec_type
  val rs1_data  = decoded.rs1_data
  val rs2_data  = decoded.rs2_data

  val oper_table = Seq(
    OperType.Rs1  -> rs1_data,
    OperType.Rs2  -> rs2_data,
    OperType.Imm  -> decoded.imm,
    OperType.Zero -> 0.U,
    OperType.Four -> 4.U,
    OperType.PC   -> decoded.pc,
    OperType.CSR  -> io.csr_rs_data
  )

  val oper1 = MuxLookup(decoded.alu_oper1_type, 0.U)(oper_table)
  val oper2 = MuxLookup(decoded.alu_oper2_type, 0.U)(oper_table)

  // ALU
  val alu = Module(new ALU)

  alu.io.oper1  := oper1
  alu.io.oper2  := oper2
  alu.io.alu_op := decoded.alu_op

  // Branch
  // Default to be `pc + imm` for  beq/bne/... and jal.
  val br_target = MuxLookup(decoded.br_op, (decoded.pc + decoded.imm).asUInt)(
    Seq(
      BrOp.Nop  -> 0.U,
      BrOp.JALR -> (rs1_data + decoded.imm)(p.XLEN - 1, 1) ## 0.U
    )
  )

  val br_taken = MuxLookup(decoded.br_op, false.B)(
    Seq(
      BrOp.Nop  -> false.B,
      BrOp.JAL  -> true.B,
      BrOp.JALR -> true.B,
      BrOp.BEQ  -> (alu.io.result === 0.U).asBool,
      BrOp.BNE  -> (alu.io.result =/= 0.U).asBool,
      BrOp.BLT  -> alu.io.result(0),
      BrOp.BGE  -> !alu.io.result(0),
      BrOp.BLTU -> alu.io.result(0),
      BrOp.BGEU -> !alu.io.result(0)
    )
  )

  // CSR
  // Use oper2 to select imm for CSR{RWI/RSI/RCI}
  io.csr_rs_addr := decoded.csr_addr
  io.csr_rs_en   := true.B

  io.out.bits.wbu.csr_rd_addr := decoded.csr_addr
  io.out.bits.wbu.csr_rd_data := MuxLookup(decoded.csr_op, 0.U)(
    Seq(
      CSROp.Nop -> 0.U,
      CSROp.RW  -> oper2,
      CSROp.RS  -> (io.csr_rs_data | oper2),
      CSROp.RC  -> (io.csr_rs_data & (~oper2).asUInt)
    )
  )

  val wbu = io.out.bits.wbu
  wbu.rd_addr  := decoded.rd_addr
  wbu.rd_we    := decoded.rd_we
  wbu.src_type := exec_type
  wbu.alu_out  := alu.io.result
  wbu.csr_out  := io.csr_rs_data

  val lsu = io.out.bits.lsu
  lsu.op         := decoded.lsu_op
  lsu.addr       := alu.io.result
  lsu.store_data := rs2_data

  // EBreak
  // val ebreak = Module(new TempEBreakForSTA)
  val ebreak = Module(new EBreak)

  ebreak.io.en    := io.in.fire && decoded.exec_type === ExecType.EBreak
  ebreak.io.clock := clock

  // Fence
  io.icache_flush := io.in.fire && decoded.exec_type === ExecType.FenceI

  // Hazard
  io.hazard.valid      := io.in.valid && decoded.rd_we
  io.hazard.rd         := decoded.rd_addr
  io.hazard.data       := MuxLookup(decoded.exec_type, 0.U)(
    Seq(
      ExecType.ALU -> alu.io.result,
      ExecType.CSR -> io.csr_rs_data
    )
  )
  io.hazard.data_valid := decoded.exec_type === ExecType.ALU || decoded.exec_type === ExecType.CSR

  io.out.bits.pc   := decoded.pc
  io.out.bits.inst := decoded.inst

  // Exception
  val prev_excp = io.in.bits.exception
  val excp      = Wire(new ExceptionInfo)

  val is_ebreak = decoded.exec_type === ExecType.EBreak
  val is_ecall  = decoded.exec_type === ExecType.ECall

  excp.valid := prev_excp.valid || is_ebreak || is_ecall
  excp.cause := MuxCase(
    0.U,
    Seq(
      prev_excp.valid -> prev_excp.cause,
      is_ebreak       -> ExceptionCode.Breakpoint,
      is_ecall        -> ExceptionCode.EnvironmentCallFromMMode
    )
  )
  excp.tval  := Mux(prev_excp.valid, prev_excp.tval, decoded.pc)

  io.out.bits.exception          := excp
  io.out.bits.wbu.is_trap_return := decoded.exec_type === ExecType.MRet

  // Jump
  io.br_valid  := io.in.fire && br_taken && !excp.valid
  io.br_target := br_target

  // IO
  io.in.ready  := io.out.ready
  io.out.valid := io.in.valid

  // Expose pc and inst in EXU to avoid the testbench see the flushed instructions
  SignalProbe(decoded.pc, "exu_pc")
  SignalProbe(decoded.inst, "exu_inst")
  SignalProbe(io.in.fire, "exu_inst_trace_ready")
  SignalProbe(Mux(io.br_valid, io.br_target, decoded.pc + 4.U), "exu_dnpc")

  def once(b: Bool) = io.in.fire && b
  PerfCounter(once(exec_type === ExecType.ALU && decoded.br_op === BrOp.Nop), "alu_ops")
  PerfCounter(once(decoded.br_op =/= BrOp.Nop), "br_ops")
  PerfCounter(once(exec_type === ExecType.LSU), "lsu_ops")
  PerfCounter(once(exec_type === ExecType.CSR), "csr_ops")
  PerfCounter(
    once(
      exec_type =/= ExecType.ALU &&
        exec_type =/= ExecType.LSU &&
        exec_type =/= ExecType.CSR
    ),
    "other_ops"
  )
  PerfCounter(once(true.B), "all_ops")
}
