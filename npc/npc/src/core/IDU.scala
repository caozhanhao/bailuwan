package core

import chisel3._
import chisel3.util._
import constants._
import top.CoreParams
import utils.Utils._

object InstDecodeTable {
  import InstPat._
  import InstFmt._
  import OperType._
  // Don't import CSR to avoid conflict
  import ExecType.{ALU, CSR => CSRInst, EBreak, ECall, LSU, MRet}

  val T = true.B
  val F = false.B

// format: off

  val default =
              //  fmt  op1   op2  WE   ALUOp       BrOp       LSUOp      CSROp   ExecType
              List(E, Zero, Zero, T, ALUOp.Add,  BrOp.Nop,  LSUOp.Nop, CSROp.Nop, ALU)
  val table = Array(
    // RV32I Base Instruction Set
    LUI    -> List(U, Imm,  Zero, T, ALUOp.Add,  BrOp.Nop,  LSUOp.Nop, CSROp.Nop, ALU),
    AUIPC  -> List(U, PC,   Imm,  T, ALUOp.Add,  BrOp.Nop,  LSUOp.Nop, CSROp.Nop, ALU),
    JAL    -> List(J, PC,   Four, T, ALUOp.Add,  BrOp.JAL,  LSUOp.Nop, CSROp.Nop, ALU),
    JALR   -> List(I, PC,   Four, T, ALUOp.Add,  BrOp.JALR, LSUOp.Nop, CSROp.Nop, ALU),
    BEQ    -> List(B, Rs1,  Rs2,  F, ALUOp.Sub,  BrOp.BEQ,  LSUOp.Nop, CSROp.Nop, ALU),
    BNE    -> List(B, Rs1,  Rs2,  F, ALUOp.Sub,  BrOp.BNE,  LSUOp.Nop, CSROp.Nop, ALU),
    BLT    -> List(B, Rs1,  Rs2,  F, ALUOp.Slt,  BrOp.BLT,  LSUOp.Nop, CSROp.Nop, ALU),
    BGE    -> List(B, Rs1,  Rs2,  F, ALUOp.Slt,  BrOp.BGE,  LSUOp.Nop, CSROp.Nop, ALU),
    BLTU   -> List(B, Rs1,  Rs2,  F, ALUOp.Sltu, BrOp.BLTU, LSUOp.Nop, CSROp.Nop, ALU),
    BGEU   -> List(B, Rs1,  Rs2,  F, ALUOp.Sltu, BrOp.BGEU, LSUOp.Nop, CSROp.Nop, ALU),
    LB     -> List(I, Rs1,  Imm,  T, ALUOp.Add,  BrOp.Nop,  LSUOp.LB,  CSROp.Nop, LSU),
    LH     -> List(I, Rs1,  Imm,  T, ALUOp.Add,  BrOp.Nop,  LSUOp.LH,  CSROp.Nop, LSU),
    LW     -> List(I, Rs1,  Imm,  T, ALUOp.Add,  BrOp.Nop,  LSUOp.LW,  CSROp.Nop, LSU),
    LBU    -> List(I, Rs1,  Imm,  T, ALUOp.Add,  BrOp.Nop,  LSUOp.LBU, CSROp.Nop, LSU),
    LHU    -> List(I, Rs1,  Imm,  T, ALUOp.Add,  BrOp.Nop,  LSUOp.LHU, CSROp.Nop, LSU),
    SB     -> List(S, Rs1,  Imm,  F, ALUOp.Add,  BrOp.Nop,  LSUOp.SB,  CSROp.Nop, LSU),
    SH     -> List(S, Rs1,  Imm,  F, ALUOp.Add,  BrOp.Nop,  LSUOp.SH,  CSROp.Nop, LSU),
    SW     -> List(S, Rs1,  Imm,  F, ALUOp.Add,  BrOp.Nop,  LSUOp.SW,  CSROp.Nop, LSU),
    ADDI   -> List(I, Rs1,  Imm,  T, ALUOp.Add,  BrOp.Nop,  LSUOp.Nop, CSROp.Nop, ALU),
    SLTI   -> List(I, Rs1,  Imm,  T, ALUOp.Slt,  BrOp.Nop,  LSUOp.Nop, CSROp.Nop, ALU),
    SLTIU  -> List(I, Rs1,  Imm,  T, ALUOp.Sltu, BrOp.Nop,  LSUOp.Nop, CSROp.Nop, ALU),
    XORI   -> List(I, Rs1,  Imm,  T, ALUOp.Xor,  BrOp.Nop,  LSUOp.Nop, CSROp.Nop, ALU),
    ORI    -> List(I, Rs1,  Imm,  T, ALUOp.Or,   BrOp.Nop,  LSUOp.Nop, CSROp.Nop, ALU),
    ANDI   -> List(I, Rs1,  Imm,  T, ALUOp.And,  BrOp.Nop,  LSUOp.Nop, CSROp.Nop, ALU),
    SLLI   -> List(I, Rs1,  Imm,  T, ALUOp.Sll,  BrOp.Nop,  LSUOp.Nop, CSROp.Nop, ALU),
    SRLI   -> List(I, Rs1,  Imm,  T, ALUOp.Srl,  BrOp.Nop,  LSUOp.Nop, CSROp.Nop, ALU),
    SRAI   -> List(I, Rs1,  Imm,  T, ALUOp.Sra,  BrOp.Nop,  LSUOp.Nop, CSROp.Nop, ALU),
    ADD    -> List(R, Rs1,  Rs2,  T, ALUOp.Add,  BrOp.Nop,  LSUOp.Nop, CSROp.Nop, ALU),
    SUB    -> List(R, Rs1,  Rs2,  T, ALUOp.Sub,  BrOp.Nop,  LSUOp.Nop, CSROp.Nop, ALU),
    SLL    -> List(R, Rs1,  Rs2,  T, ALUOp.Sll,  BrOp.Nop,  LSUOp.Nop, CSROp.Nop, ALU),
    SLT    -> List(R, Rs1,  Rs2,  T, ALUOp.Slt,  BrOp.Nop,  LSUOp.Nop, CSROp.Nop, ALU),
    SLTU   -> List(R, Rs1,  Rs2,  T, ALUOp.Sltu, BrOp.Nop,  LSUOp.Nop, CSROp.Nop, ALU),
    XOR    -> List(R, Rs1,  Rs2,  T, ALUOp.Xor,  BrOp.Nop,  LSUOp.Nop, CSROp.Nop, ALU),
    SRL    -> List(R, Rs1,  Rs2,  T, ALUOp.Srl,  BrOp.Nop,  LSUOp.Nop, CSROp.Nop, ALU),
    SRA    -> List(R, Rs1,  Rs2,  T, ALUOp.Sra,  BrOp.Nop,  LSUOp.Nop, CSROp.Nop, ALU),
    OR     -> List(R, Rs1,  Rs2,  T, ALUOp.Or,   BrOp.Nop,  LSUOp.Nop, CSROp.Nop, ALU),
    AND    -> List(R, Rs1,  Rs2,  T, ALUOp.And,  BrOp.Nop,  LSUOp.Nop, CSROp.Nop, ALU),
    ECALL  -> List(I, Zero, Zero, F, ALUOp.Add,  BrOp.Nop,  LSUOp.Nop, CSROp.Nop, ECall),
    EBREAK -> List(I, Zero, Zero, F, ALUOp.Add,  BrOp.Nop,  LSUOp.Nop, CSROp.Nop, EBreak),

    // RV32/RV64 Zicsr Standard Extension
    CSRRW  -> List(C, CSR,  Rs1,  T, ALUOp.Add,  BrOp.Nop,  LSUOp.Nop, CSROp.RW,  CSRInst),
    CSRRS  -> List(C, CSR,  Rs1,  T, ALUOp.Add,  BrOp.Nop,  LSUOp.Nop, CSROp.RS,  CSRInst),
    CSRRC  -> List(C, CSR,  Rs1,  T, ALUOp.Add,  BrOp.Nop,  LSUOp.Nop, CSROp.RC,  CSRInst),
    CSRRWI -> List(C, CSR,  Imm,  T, ALUOp.Add,  BrOp.Nop,  LSUOp.Nop, CSROp.RW,  CSRInst),
    CSRRSI -> List(C, CSR,  Imm,  T, ALUOp.Add,  BrOp.Nop,  LSUOp.Nop, CSROp.RS,  CSRInst),
    CSRRCI -> List(C, CSR,  Imm,  T, ALUOp.Add,  BrOp.Nop,  LSUOp.Nop, CSROp.RC,  CSRInst),

    // Trap-Return Instructions
    MRET   -> List(I, Zero, Zero, F, ALUOp.Add,  BrOp.Nop,  LSUOp.Nop, CSROp.Nop, MRet),

    // TODO: FENCE?
  )

// format: on
}

class IDUOut(
  implicit p: CoreParams)
    extends Bundle {
  val pc = UInt(p.XLEN.W)

  val alu_oper1_type = UInt(OperType.WIDTH)
  val alu_oper2_type = UInt(OperType.WIDTH)

  val rs1_data = UInt(p.XLEN.W)
  val rs2_data = UInt(p.XLEN.W)
  val rd_we    = Bool()
  val imm      = UInt(p.XLEN.W)
  val csr_addr = UInt(12.W)

  val exec_type = UInt(ExecType.WIDTH)
  val alu_op    = UInt(ALUOp.WIDTH)
  val lsu_op    = UInt(LSUOp.WIDTH)
  val br_op     = UInt(BrOp.WIDTH)
  val csr_op    = UInt(CSROp.WIDTH)
}

class IDURegfileIn(
  implicit p: CoreParams)
    extends Bundle {
  val rs1_data = UInt(p.XLEN.W)
  val rs2_data = UInt(p.XLEN.W)
}

class IDURegfileOut extends Bundle {
  val rs1_addr = UInt(5.W)
  val rs2_addr = UInt(5.W)
  val rd_addr  = UInt(5.W)
}

class IDU(
  implicit p: CoreParams)
    extends Module {
  val io = IO(new Bundle {
    val in  = Flipped(Decoupled(new IFUOut))
    val out = Decoupled(new IDUOut)

    val regfile_in  = Input(new IDURegfileIn)
    val regfile_out = Output(new IDURegfileOut)
  })

  val NOP  = 0x00000013.U(32.W)
  val inst = Mux(io.in.valid, io.in.bits.inst, NOP)

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

  assert(fmt =/= InstFmt.E, cf"Invalid instruction format. (Inst: ${inst})")

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

  // printf(cf"[IDU]: Inst: ${inst}, imm: ${imm}, rd: ${rd}, rs1: ${rs1}, rs2: ${rs2}, exec_type: ${exec_type}\n");

  // IO
  io.out.bits.pc             := io.in.bits.pc
  io.out.bits.alu_oper1_type := oper1_type
  io.out.bits.alu_oper2_type := oper2_type
  io.out.bits.imm            := imm
  io.out.bits.csr_addr       := csr_addr
  io.out.bits.alu_op         := alu_op
  io.out.bits.lsu_op         := lsu_op
  io.out.bits.br_op          := br_op
  io.out.bits.exec_type      := exec_type
  io.out.bits.csr_op         := csr_op

  // Regfile
  io.regfile_out.rs1_addr := rs1
  io.regfile_out.rs2_addr := rs2
  io.regfile_out.rd_addr  := rd
  io.out.bits.rs1_data    := io.regfile_in.rs1_data
  io.out.bits.rs2_data    := io.regfile_in.rs2_data
  io.out.bits.rd_we       := we

  io.in.ready  := io.out.ready
  io.out.valid := io.in.valid
}
