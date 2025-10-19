package core

import chisel3._
import chisel3.util._
import constants.CSR
import top.CoreParams

class CSRFile(
  implicit p: CoreParams)
    extends Module {
  val io = IO(new Bundle {
    val read_addr   = Input(UInt(5.W))
    val read_enable = Input(Bool())
    val read_data   = Output(UInt(p.XLEN.W))

    val write_addr   = Input(UInt(5.W))
    val write_data   = Input(UInt(p.XLEN.W))
    val write_enbale = Input(Bool())
  })

  val mstatus_init_val = if (p.XLEN == 32) 0x1800.U(32.W) else 0xa00001800L.U(64.W)

  val mstatus = RegInit(mstatus_init_val)
  val mtvec   = RegInit(0.U(p.XLEN.W))
  val mepc    = RegInit(0.U(p.XLEN.W))
  val mcause  = RegInit(0.U(p.XLEN.W))
  val mcycle  = RegInit(0.U(p.XLEN.W))

  val mcycleh = if (p.XLEN == 32) RegInit(0.U(p.XLEN.W)) else null

  if (p.XLEN == 32) {
    mcycle  := Mux(mcycle(31), 0.U, mcycle + 1.U)
    mcycleh := Mux(mcycle(31), mcycleh + 1.U, mcycleh)
  } else {
    mcycle := mcycle + 1.U
  }

  def writable(addr: UInt, old_val: UInt) = Mux(io.write_enbale && (io.write_addr === addr), io.write_data, old_val)

  mstatus := writable(CSR.mstatus, mstatus)
  mtvec   := writable(CSR.mtvec, mtvec)
  mepc    := writable(CSR.mepc, mepc)
  mcause  := writable(CSR.mcause, mcause)
  mcycle  := writable(CSR.mcycle, mcycle)

  if (p.XLEN == 32)
    mcycleh := writable(CSR.mcycleh, mcycleh)

  val read_data = MuxLookup(io.read_addr, 0.U)(
    Seq(
      CSR.mstatus   -> mstatus,
      CSR.mtvec     -> mtvec,
      CSR.mepc      -> mepc,
      CSR.mcause    -> mcause,
      CSR.mcycle    -> mcycle,
      CSR.mcycleh   -> (if (p.XLEN == 32) mcycleh else 0.U),
      // ysyx_25100251 caozhanhao
      CSR.mvendorid -> 0x79737978.U,
      CSR.marchid   -> 25100251.U
    )
  )

  io.read_data := Mux(io.read_enable, read_data, 0.U)
}
