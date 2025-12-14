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
    val exu_hazard_rd       = Input(UInt(5.W))
    val exu_hazard_rd_valid = Input(Bool())
    val lsu_hazard_rd       = Input(UInt(5.W))
    val lsu_hazard_rd_valid = Input(Bool())
    val wbu_hazard_rd       = Input(UInt(5.W))
    val wbu_hazard_rd_valid = Input(Bool())
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

  // CSR Addr:
  //   CSR{RW, RS, RC}[I] -> the CSR indicated by inst[31:20]
  //   ECall              -> mtvec
  //   MRet               -> mepc
  val csr_addr = MuxLookup(exec_type, inst(31, 20))(
    Seq(
      ExecType.ECall -> CSR.mtvec,
      ExecType.MRet  -> CSR.mepc
    )
  )

  // Detecting Hazards
  // Don't only use oper_type to detect hazards, because jump/branch's
  // ALU Op is always pc + 4 or branch cond.
  val rs1_read = fmt === InstFmt.R || fmt === InstFmt.I ||
    fmt === InstFmt.S || fmt === InstFmt.B ||
    // Special case for CSR Insts
    (fmt === InstFmt.C && oper2_type === OperType.Rs1)

  val rs2_read = fmt === InstFmt.R || fmt === InstFmt.S || fmt === InstFmt.B

  def has_hazard(rs: UInt, read: Bool) =
    rs =/= 0.U && read &&
      ((rs === io.exu_hazard_rd && io.exu_hazard_rd_valid)
        || (rs === io.lsu_hazard_rd && io.lsu_hazard_rd_valid)
        || (rs === io.wbu_hazard_rd && io.wbu_hazard_rd_valid))

  val wait_ebreak = exec_type === ExecType.EBreak && (
    io.exu_hazard_rd_valid || io.lsu_hazard_rd_valid || io.wbu_hazard_rd_valid
  )

  val hazard = io.in.valid && (has_hazard(rs1, rs1_read) || has_hazard(rs2, rs2_read) || wait_ebreak)

  // Exception
  val prev_excp  = io.in.bits.exception
  val excp       = Wire(new ExceptionInfo)
  val is_illegal = fmt === InstFmt.E

  excp.valid := prev_excp.valid || is_illegal
  excp.cause := Mux(prev_excp.valid, prev_excp.cause, ExceptionCode.IllegalInstruction)
  excp.tval  := Mux(prev_excp.valid, prev_excp.tval, inst)

  // IO
  io.out.bits.pc             := pc
  io.out.bits.inst           := inst
  io.out.bits.alu_oper1_type := oper1_type
  io.out.bits.alu_oper2_type := oper2_type
  io.out.bits.imm            := imm
  io.out.bits.csr_addr       := csr_addr
  io.out.bits.alu_op         := alu_op
  io.out.bits.lsu_op         := lsu_op
  io.out.bits.br_op          := br_op
  io.out.bits.exec_type      := exec_type
  io.out.bits.csr_op         := csr_op
  io.out.bits.rd_addr        := rd
  io.out.bits.rd_we          := we
  io.out.bits.exception      := excp

  // Regfile
  io.rs1_addr          := rs1
  io.rs2_addr          := rs2
  io.out.bits.rs1_data := io.rs1_data
  io.out.bits.rs2_data := io.rs2_data

  io.in.ready  := io.out.ready && !hazard
  io.out.valid := io.in.valid && !hazard

  SignalProbe(pc, "idu_pc")
  SignalProbe(inst, "idu_inst")
}
