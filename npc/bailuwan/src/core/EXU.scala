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
  val alu_out = UInt(p.XLEN.W)
  // CSR Out:
  //   CSR{RW, RS, RC}[I] -> the CSR indicated by inst[31:20]
  //   ECall              -> mtvec
  //   MRet               -> mepc
  val csr_out = UInt(p.XLEN.W)
}

class EXUOutForLSU(
  implicit p: CoreParams)
    extends Bundle {
  val lsu_op         = UInt(LSUOp.WIDTH)
  val lsu_addr       = UInt(p.XLEN.W)
  val lsu_store_data = UInt(p.XLEN.W)

  val pc   = if (p.Debug) Some(UInt(p.XLEN.W)) else None
  val inst = if (p.Debug) Some(UInt(32.W)) else None
}

class EXUOut(
  implicit p: CoreParams)
    extends Bundle {
  val lsu = new EXUOutForLSU
  val wbu = new EXUOutForWBU
}

class EXU(
  implicit p: CoreParams)
    extends Module {
  val io = IO(new Bundle {
    val in  = Flipped(Decoupled(new IDUOut))
    val out = Decoupled(new EXUOut)

    // IFU
    val redirect_valid  = Output(Bool())
    val redirect_target = Output(UInt(p.XLEN.W))

    // ICache
    val icache_flush = Output(Bool())

    // Hazard
    val rd       = Output(UInt(5.W))
    val rd_valid = Output(Bool())
  })

  val decoded = io.in.bits

  val exec_type = decoded.exec_type
  val rs1_data  = decoded.rs1_data
  val rs2_data  = decoded.rs2_data

  val csr_file = Module(new CSRFile)
  csr_file.io.read_addr   := decoded.csr_addr
  csr_file.io.read_enable := true.B

  csr_file.io.write_addr   := decoded.csr_addr
  csr_file.io.write_enable := exec_type === ExecType.CSR

  // FIXME
  csr_file.io.has_intr := (exec_type === ExecType.ECall)
  csr_file.io.epc      := decoded.pc
  csr_file.io.cause    := MuxLookup(exec_type, 0.U)(
    Seq(
      // ECall from M priv.
      ExecType.ECall -> (0x3 + 8).U
    )
  )

  val csr_data = csr_file.io.read_data

  val oper_table = Seq(
    OperType.Rs1  -> rs1_data,
    OperType.Rs2  -> rs2_data,
    OperType.Imm  -> decoded.imm,
    OperType.Zero -> 0.U,
    OperType.Four -> 4.U,
    OperType.PC   -> decoded.pc,
    OperType.CSR  -> csr_data
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
  val csr_write_data = MuxLookup(decoded.csr_op, 0.U)(
    Seq(
      CSROp.Nop -> 0.U,
      CSROp.RW  -> oper2,
      CSROp.RS  -> (csr_data | oper2),
      CSROp.RC  -> (csr_data & (~oper2).asUInt)
    )
  )

  // printf(cf"[EXU/CSR]: op=${decoded.csr_op}, oper2=${oper2}, csr_addr=${decoded.csr_addr}, csr_data=${csr_data}, rd=${decoded.rd}, rd_we=${decoded.rd_we}\n")

  csr_file.io.write_data := csr_write_data

  val wbu = io.out.bits.wbu
  wbu.rd_addr  := decoded.rd_addr
  wbu.rd_we    := decoded.rd_we
  wbu.src_type := exec_type
  wbu.alu_out  := alu.io.result
  wbu.csr_out  := csr_data

  val lsu = io.out.bits.lsu
  lsu.lsu_op         := decoded.lsu_op
  lsu.lsu_addr       := alu.io.result
  lsu.lsu_store_data := rs2_data

  // EBreak
  // val ebreak = Module(new TempEBreakForSTA)
  val ebreak = Module(new EBreak)

  ebreak.io.en    := decoded.exec_type === ExecType.EBreak
  ebreak.io.clock := clock

  // Fence
  io.icache_flush := decoded.exec_type === ExecType.FenceI

  // dnpc
  io.redirect_valid  := io.in.valid && (br_taken || exec_type === ExecType.ECall || exec_type === ExecType.MRet)
  io.redirect_target := MuxLookup(exec_type, br_target)(
    Seq(
      ExecType.ECall -> csr_data,
      ExecType.MRet  -> csr_data
    )
  )

  // Hazard
  io.rd       := decoded.rd_addr
  io.rd_valid := io.in.valid && decoded.rd_we

  // Optional Debug Signals
  io.out.bits.lsu.pc.foreach { i => i := decoded.pc }
  io.out.bits.lsu.inst.foreach { i => i := decoded.inst }

  io.in.ready  := io.out.ready
  io.out.valid := io.in.valid

  // Expose pc and inst in EXU to avoid the testbench see the flushed instructions
  SignalProbe(decoded.pc, "pc")
  SignalProbe(decoded.inst, "inst")
  SignalProbe(io.in.fire, "inst_trace_ready")

  PerfCounter(io.out.valid, "exu_done")
}
