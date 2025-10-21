package core

import chisel3._
import chisel3.util._

import constants._

import top.CoreParams

class EXUOut(
  implicit p: CoreParams)
    extends Bundle {
  val src_type = UInt(ExecType.WIDTH)
  val alu_out  = UInt(p.XLEN.W)
  val lsu_out  = UInt(p.XLEN.W)

  // CSR Out:
  //   CSR{RW, RS, RC}[I] -> the CSR indicated by inst[31:20]
  //   ECall              -> mtvec
  //   MRet               -> mepc
  val csr_out = UInt(p.XLEN.W)

  // PC
  val snpc      = UInt(p.XLEN.W)
  val br_taken  = Bool()
  val br_target = UInt(p.XLEN.W)
}

class EXU(
  implicit p: CoreParams)
    extends Module {
  val io = IO(new Bundle {
    val in  = Flipped(Decoupled(new IDUOut))
    val out = Decoupled(new EXUOut)
  })

  val decoded   = io.in.bits
  val exec_type = decoded.exec_type
  val rs1_data  = decoded.rs1_data
  val rs2_data  = decoded.rs2_data

  val csr_file = Module(new CSRFile)
  csr_file.io.read_addr   := decoded.csr_addr
  csr_file.io.read_enable := true.B

  csr_file.io.write_addr   := decoded.csr_addr
  csr_file.io.write_enbale := exec_type === ExecType.CSR

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

  // LSU
  val lsu = Module(new LSU)
  lsu.io.lsu_op     := decoded.lsu_op
  lsu.io.addr       := alu.io.result
  lsu.io.write_data := rs2_data

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

  io.out.bits.src_type := exec_type
  io.out.bits.alu_out  := alu.io.result
  io.out.bits.lsu_out  := lsu.io.read_data
  io.out.bits.csr_out  := csr_data

  io.out.bits.snpc      := decoded.pc + 4.U
  io.out.bits.br_taken  := br_taken
  io.out.bits.br_target := br_target

  // EBreak
  // val ebreak = Module(new TempEBreakForSTA)
  val ebreak = Module(new EBreak)

  ebreak.io.en := decoded.exec_type === ExecType.EBreak

  io.in.ready  := io.out.ready
  io.out.valid := io.in.valid
}
