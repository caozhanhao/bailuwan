package core

import chisel3._
import chisel3.util._

import constants._

import top.CoreParams

class EXU(
  implicit p: CoreParams)
    extends Module {
  val io = IO(new Bundle {
    val pc      = Input(UInt(p.XLEN.W))
    val decoded = Input(new DecodedBundle)
    val dnpc    = Output(UInt(p.XLEN.W))
  })

  val exec_type = io.decoded.exec_type

  val reg_file = Module(new RegFile)
  reg_file.io.rs1_addr := io.decoded.rs1
  reg_file.io.rs2_addr := io.decoded.rs2
  reg_file.io.rd_addr  := io.decoded.rd
  reg_file.io.rd_we    := io.decoded.rd_we

  val rs1_data = reg_file.io.rs1_data
  val rs2_data = reg_file.io.rs2_data

  val csr_file = Module(new CSRFile)
  csr_file.io.read_addr   := io.decoded.csr_addr
  csr_file.io.read_enable := true.B

  csr_file.io.write_addr   := io.decoded.csr_addr
  csr_file.io.write_enbale := exec_type === ExecType.CSR

  val csr_data = csr_file.io.read_data

  val oper_table = Seq(
    OperType.Rs1  -> rs1_data,
    OperType.Rs2  -> rs2_data,
    OperType.Imm  -> io.decoded.imm,
    OperType.Zero -> 0.U,
    OperType.Four -> 4.U,
    OperType.PC   -> io.pc,
    OperType.CSR  -> csr_data
  )

  val oper1 = MuxLookup(io.decoded.alu_oper1_type, 0.U)(oper_table)
  val oper2 = MuxLookup(io.decoded.alu_oper2_type, 0.U)(oper_table)

  // ALU
  val alu = Module(new ALU)

  alu.io.oper1  := oper1
  alu.io.oper2  := oper2
  alu.io.alu_op := io.decoded.alu_op

  // LSU
  val lsu = Module(new LSU)
  lsu.io.lsu_op     := io.decoded.lsu_op
  lsu.io.addr       := alu.io.result
  lsu.io.write_data := rs2_data

  // Branch
  // Default to be `pc + imm` for  beq/bne/... and jal.
  val br_target = MuxLookup(io.decoded.br_op, (io.pc + io.decoded.imm).asUInt)(
    Seq(
      BrOp.Nop  -> 0.U,
      BrOp.JALR -> (reg_file.io.rs1_data + io.decoded.imm)(p.XLEN - 1, 1) ## 0.U
    )
  )

  val br_taken = MuxLookup(io.decoded.br_op, false.B)(
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
  val csr_write_data = MuxLookup(io.decoded.csr_op, 0.U)(
    Seq(
      CSROp.Nop -> 0.U,
      CSROp.RW  -> oper2,
      CSROp.RS  -> (oper1 | oper2),
      CSROp.RC  -> (oper1 & (~oper2).asUInt)
    )
  )
  
  csr_file.io.write_data := csr_write_data

  // Write Back
  val wbu = Module(new WBU)

  wbu.io.in.src_type := exec_type
  wbu.io.in.alu_out  := alu.io.result
  wbu.io.in.lsu_out  := lsu.io.read_data
  wbu.io.in.csr_out  := csr_data

  wbu.io.in.snpc      := io.pc + 4.U
  wbu.io.in.br_taken  := br_taken
  wbu.io.in.br_target := br_target

  reg_file.io.rd_data := wbu.io.out.rd_data
  io.dnpc             := wbu.io.out.dnpc

  // EBreak
  val ebreak = Module(new EBreak)
  ebreak.io.en := io.decoded.exec_type === ExecType.EBreak
}
