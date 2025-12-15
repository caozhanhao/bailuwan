// Copyright (c) 2025 caozhanhao
// SPDX-License-Identifier: MIT

package core

import chisel3._
import chisel3.util._
import constants._
import utils.{PerfCounter, SignalProbe}
import utils.Utils._
import bailuwan.CoreParams

object InstDecodeTable {
  import InstPat._
  import InstFmt._
  import OperType._
  // Don't import CSR to avoid conflict
  import ExecType.{ALU, CSR => CSRInst, EBreak, ECall, FenceI, LSU, MRet}

  val T = true.B
  val F = false.B

// format: off

  val default =
               //  fmt  op1   op2  WE   ALUOp       BrOp       LSUOp      CSROp   ExecType
               List(E, Zero, Zero, T, ALUOp.Add,  BrOp.Nop,  LSUOp.Nop, CSROp.Nop, ALU)
  val table = Array(
    // RV32I Base Instruction Set
    LUI     -> List(U, Imm,  Zero, T, ALUOp.Add,  BrOp.Nop,  LSUOp.Nop, CSROp.Nop, ALU),
    AUIPC   -> List(U, PC,   Imm,  T, ALUOp.Add,  BrOp.Nop,  LSUOp.Nop, CSROp.Nop, ALU),
    JAL     -> List(J, PC,   Four, T, ALUOp.Add,  BrOp.JAL,  LSUOp.Nop, CSROp.Nop, ALU),
    JALR    -> List(I, PC,   Four, T, ALUOp.Add,  BrOp.JALR, LSUOp.Nop, CSROp.Nop, ALU),
    BEQ     -> List(B, Rs1,  Rs2,  F, ALUOp.Sub,  BrOp.BEQ,  LSUOp.Nop, CSROp.Nop, ALU),
    BNE     -> List(B, Rs1,  Rs2,  F, ALUOp.Sub,  BrOp.BNE,  LSUOp.Nop, CSROp.Nop, ALU),
    BLT     -> List(B, Rs1,  Rs2,  F, ALUOp.Slt,  BrOp.BLT,  LSUOp.Nop, CSROp.Nop, ALU),
    BGE     -> List(B, Rs1,  Rs2,  F, ALUOp.Slt,  BrOp.BGE,  LSUOp.Nop, CSROp.Nop, ALU),
    BLTU    -> List(B, Rs1,  Rs2,  F, ALUOp.Sltu, BrOp.BLTU, LSUOp.Nop, CSROp.Nop, ALU),
    BGEU    -> List(B, Rs1,  Rs2,  F, ALUOp.Sltu, BrOp.BGEU, LSUOp.Nop, CSROp.Nop, ALU),
    LB      -> List(I, Rs1,  Imm,  T, ALUOp.Add,  BrOp.Nop,  LSUOp.LB,  CSROp.Nop, LSU),
    LH      -> List(I, Rs1,  Imm,  T, ALUOp.Add,  BrOp.Nop,  LSUOp.LH,  CSROp.Nop, LSU),
    LW      -> List(I, Rs1,  Imm,  T, ALUOp.Add,  BrOp.Nop,  LSUOp.LW,  CSROp.Nop, LSU),
    LBU     -> List(I, Rs1,  Imm,  T, ALUOp.Add,  BrOp.Nop,  LSUOp.LBU, CSROp.Nop, LSU),
    LHU     -> List(I, Rs1,  Imm,  T, ALUOp.Add,  BrOp.Nop,  LSUOp.LHU, CSROp.Nop, LSU),
    SB      -> List(S, Rs1,  Imm,  F, ALUOp.Add,  BrOp.Nop,  LSUOp.SB,  CSROp.Nop, LSU),
    SH      -> List(S, Rs1,  Imm,  F, ALUOp.Add,  BrOp.Nop,  LSUOp.SH,  CSROp.Nop, LSU),
    SW      -> List(S, Rs1,  Imm,  F, ALUOp.Add,  BrOp.Nop,  LSUOp.SW,  CSROp.Nop, LSU),
    ADDI    -> List(I, Rs1,  Imm,  T, ALUOp.Add,  BrOp.Nop,  LSUOp.Nop, CSROp.Nop, ALU),
    SLTI    -> List(I, Rs1,  Imm,  T, ALUOp.Slt,  BrOp.Nop,  LSUOp.Nop, CSROp.Nop, ALU),
    SLTIU   -> List(I, Rs1,  Imm,  T, ALUOp.Sltu, BrOp.Nop,  LSUOp.Nop, CSROp.Nop, ALU),
    XORI    -> List(I, Rs1,  Imm,  T, ALUOp.Xor,  BrOp.Nop,  LSUOp.Nop, CSROp.Nop, ALU),
    ORI     -> List(I, Rs1,  Imm,  T, ALUOp.Or,   BrOp.Nop,  LSUOp.Nop, CSROp.Nop, ALU),
    ANDI    -> List(I, Rs1,  Imm,  T, ALUOp.And,  BrOp.Nop,  LSUOp.Nop, CSROp.Nop, ALU),
    SLLI    -> List(I, Rs1,  Imm,  T, ALUOp.Sll,  BrOp.Nop,  LSUOp.Nop, CSROp.Nop, ALU),
    SRLI    -> List(I, Rs1,  Imm,  T, ALUOp.Srl,  BrOp.Nop,  LSUOp.Nop, CSROp.Nop, ALU),
    SRAI    -> List(I, Rs1,  Imm,  T, ALUOp.Sra,  BrOp.Nop,  LSUOp.Nop, CSROp.Nop, ALU),
    ADD     -> List(R, Rs1,  Rs2,  T, ALUOp.Add,  BrOp.Nop,  LSUOp.Nop, CSROp.Nop, ALU),
    SUB     -> List(R, Rs1,  Rs2,  T, ALUOp.Sub,  BrOp.Nop,  LSUOp.Nop, CSROp.Nop, ALU),
    SLL     -> List(R, Rs1,  Rs2,  T, ALUOp.Sll,  BrOp.Nop,  LSUOp.Nop, CSROp.Nop, ALU),
    SLT     -> List(R, Rs1,  Rs2,  T, ALUOp.Slt,  BrOp.Nop,  LSUOp.Nop, CSROp.Nop, ALU),
    SLTU    -> List(R, Rs1,  Rs2,  T, ALUOp.Sltu, BrOp.Nop,  LSUOp.Nop, CSROp.Nop, ALU),
    XOR     -> List(R, Rs1,  Rs2,  T, ALUOp.Xor,  BrOp.Nop,  LSUOp.Nop, CSROp.Nop, ALU),
    SRL     -> List(R, Rs1,  Rs2,  T, ALUOp.Srl,  BrOp.Nop,  LSUOp.Nop, CSROp.Nop, ALU),
    SRA     -> List(R, Rs1,  Rs2,  T, ALUOp.Sra,  BrOp.Nop,  LSUOp.Nop, CSROp.Nop, ALU),
    OR      -> List(R, Rs1,  Rs2,  T, ALUOp.Or,   BrOp.Nop,  LSUOp.Nop, CSROp.Nop, ALU),
    AND     -> List(R, Rs1,  Rs2,  T, ALUOp.And,  BrOp.Nop,  LSUOp.Nop, CSROp.Nop, ALU),
    ECALL   -> List(I, Zero, Zero, F, ALUOp.Add,  BrOp.Nop,  LSUOp.Nop, CSROp.Nop, ECall),
    EBREAK  -> List(I, Zero, Zero, F, ALUOp.Add,  BrOp.Nop,  LSUOp.Nop, CSROp.Nop, EBreak),

    // RV32/RV64 Zicsr Standard Extension
    CSRRW   -> List(C, CSR,  Rs1,  T, ALUOp.Add,  BrOp.Nop,  LSUOp.Nop, CSROp.RW,  CSRInst),
    CSRRS   -> List(C, CSR,  Rs1,  T, ALUOp.Add,  BrOp.Nop,  LSUOp.Nop, CSROp.RS,  CSRInst),
    CSRRC   -> List(C, CSR,  Rs1,  T, ALUOp.Add,  BrOp.Nop,  LSUOp.Nop, CSROp.RC,  CSRInst),
    CSRRWI  -> List(C, CSR,  Imm,  T, ALUOp.Add,  BrOp.Nop,  LSUOp.Nop, CSROp.RW,  CSRInst),
    CSRRSI  -> List(C, CSR,  Imm,  T, ALUOp.Add,  BrOp.Nop,  LSUOp.Nop, CSROp.RS,  CSRInst),
    CSRRCI  -> List(C, CSR,  Imm,  T, ALUOp.Add,  BrOp.Nop,  LSUOp.Nop, CSROp.RC,  CSRInst),

    // Trap-Return Instructions
    MRET    -> List(I, Zero, Zero, F, ALUOp.Add,  BrOp.Nop,  LSUOp.Nop, CSROp.Nop, MRet),

    // RV32/RV64 Zifencei Standard Extension
    FENCE_I -> List(I, Zero, Zero, F, ALUOp.Add,  BrOp.Nop,  LSUOp.Nop, CSROp.Nop, FenceI),
  )

// format: on
}

class IDUOut(
  implicit p: CoreParams)
    extends Bundle {
  val pc   = UInt(p.XLEN.W)
  val inst = UInt(32.W)

  val alu_oper1_type = UInt(OperType.WIDTH)
  val alu_oper2_type = UInt(OperType.WIDTH)

  val rs1_data = UInt(p.XLEN.W)
  val rs2_data = UInt(p.XLEN.W)
  val rd_addr  = UInt(5.W)
  val rd_we    = Bool()
  val imm      = UInt(p.XLEN.W)
  val csr_addr = UInt(12.W)

  val exec_type = UInt(ExecType.WIDTH)
  val alu_op    = UInt(ALUOp.WIDTH)
  val lsu_op    = UInt(LSUOp.WIDTH)
  val br_op     = UInt(BrOp.WIDTH)
  val csr_op    = UInt(CSROp.WIDTH)

  val exception = new ExceptionInfo

  val predict_taken  = Bool()
  val predict_target = UInt(p.XLEN.W)
}

class HazardInfo(
  implicit p: CoreParams)
    extends Bundle {
  val valid      = Bool()
  val rd         = UInt(5.W)
  val data       = UInt(p.XLEN.W)
  val data_valid = Bool()
}

class IDU(
  implicit p: CoreParams)
    extends Module {
  val io = IO(new Bundle {
    val in  = Flipped(Decoupled(new IFUOut))
    val out = Decoupled(new IDUOut)

    // RegFile
    val rs1_data = Input(UInt(p.XLEN.W))
    val rs2_data = Input(UInt(p.XLEN.W))
    val rs1_addr = Output(UInt(5.W))
    val rs2_addr = Output(UInt(5.W))

    // Hazard
    val exu_hazard = Input(new HazardInfo)
    val lsu_hazard = Input(new HazardInfo)
    val wbu_hazard = Input(new HazardInfo)
  })

  val pc   = io.in.bits.pc
  val inst = io.in.bits.inst

  // Registers
  val rd  = inst(11, 7)
  val rs1 = inst(19, 15)
  val rs2 = inst(24, 20)

  // Immediates
  val immI   = sign_extend(inst(31, 20), 32)
  val immS   = sign_extend(inst(31, 25) ## inst(11, 7), 32)
  val immB   = sign_extend(inst(31) ## inst(7) ## inst(30, 25) ## inst(11, 8) ## 0.U(1.W), 32)
  val immU   = inst(31, 12) ## Fill(12, 0.U)
  val immJ   = sign_extend(inst(31) ## inst(19, 12) ## inst(20) ## inst(30, 21) ## 0.U(1.W), 32)
  val immCsr = inst(19, 15)

  // Decode
  val fmt :: oper1_type :: oper2_type :: (we: Bool) :: alu_op :: br_op :: lsu_op :: csr_op :: exec_type :: Nil =
    ListLookup(inst, InstDecodeTable.default, InstDecodeTable.table)

  // Choose immediate
  val imm = MuxLookup(fmt, 0.U)(
    Seq(
      InstFmt.I -> immI,
      InstFmt.S -> immS,
      InstFmt.B -> immB,
      InstFmt.U -> immU,
      InstFmt.J -> immJ,
      InstFmt.C -> immCsr
    )
  )

  val csr_addr = inst(31, 20)

  // Resolving Hazards
  val hazard_info = Seq(io.exu_hazard, io.lsu_hazard, io.wbu_hazard)

  def forward_reg(rs: UInt, regfile_data: UInt) = {
    val cases = hazard_info.map(info => (rs =/= 0.U && info.valid && info.rd === rs && info.data_valid) -> info.data)
    MuxCase(regfile_data, cases)
  }

  val rs1_decoded = forward_reg(rs1, io.rs1_data)
  val rs2_decoded = forward_reg(rs2, io.rs2_data)

  // Don't only use oper_type to detect hazards, because jump/branch's
  // ALU Op is always pc + 4 or branch cond.
  val rs1_read = fmt === InstFmt.R || fmt === InstFmt.I ||
    fmt === InstFmt.S || fmt === InstFmt.B ||
    // Special case for CSR Insts
    (fmt === InstFmt.C && oper2_type === OperType.Rs1)

  val rs2_read = fmt === InstFmt.R || fmt === InstFmt.S || fmt === InstFmt.B

  def need_stall(rs: UInt, read: Bool) = {
    // Hazard happens but data not valid
    val stall_info = hazard_info.map { info => info.valid && info.rd === rs && !info.data_valid }
    rs =/= 0.U && read && stall_info.reduce(_ || _)
  }

  val wait_ebreak = exec_type === ExecType.EBreak &&
    hazard_info.map(info => info.valid).reduce(_ || _)

  val hazard_stall = io.in.valid && (need_stall(rs1, rs1_read) || need_stall(rs2, rs2_read) || wait_ebreak)

  // Exception
  val prev_excp  = io.in.bits.exception
  val excp       = Wire(new ExceptionInfo)
  val is_illegal = fmt === InstFmt.E

  excp.valid := prev_excp.valid || is_illegal
  excp.cause := Mux(prev_excp.valid, prev_excp.cause, ExceptionCode.IllegalInstruction)
  excp.tval  := Mux(prev_excp.valid, prev_excp.tval, inst)

  // IO
  val out = Wire(new IDUOut)
  out.pc             := pc
  out.inst           := inst
  out.rs1_data       := rs1_decoded
  out.rs2_data       := rs2_decoded
  out.alu_oper1_type := oper1_type
  out.alu_oper2_type := oper2_type
  out.imm            := imm
  out.csr_addr       := csr_addr
  out.alu_op         := alu_op
  out.lsu_op         := lsu_op
  out.br_op          := br_op
  out.exec_type      := exec_type
  out.csr_op         := csr_op
  out.rd_addr        := rd
  out.rd_we          := we
  out.exception      := excp
  out.predict_taken  := io.in.bits.predict_taken
  out.predict_target := io.in.bits.predict_target

  io.out.bits := out

  // Regfile
  io.rs1_addr := rs1
  io.rs2_addr := rs2

  io.in.ready  := io.out.ready && !hazard_stall
  io.out.valid := io.in.valid && !hazard_stall

  SignalProbe(pc, "idu_pc")
  SignalProbe(inst, "idu_inst")
  PerfCounter(hazard_stall, "idu_hazard_stall_cycles")
}
