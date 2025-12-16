// Copyright (c) 2025 caozhanhao
// SPDX-License-Identifier: MIT

package core

import chisel3._
import chisel3.util._
import bailuwan.CoreParams

object BranchType {
  val WIDTH = log2Ceil(4).W

  val Call   = 0.U(WIDTH)
  val Ret    = 1.U(WIDTH)
  val Jump   = 2.U(WIDTH)
  val Branch = 3.U(WIDTH)
}

class BTBReadIO(
  implicit p: CoreParams)
    extends Bundle {
  val pc = Input(UInt(p.XLEN.W))

  val valid  = Output(Bool())
  val target = Output(UInt(p.XLEN.W))
  val btype  = Output(UInt(BranchType.WIDTH))
}

class BTBWriteIO(
  implicit p: CoreParams)
    extends Bundle {
  val en     = Input(Bool())
  val pc     = Input(UInt(p.XLEN.W))
  val target = Input(UInt(p.XLEN.W))
  val btype  = Input(UInt(BranchType.WIDTH))
}

// Simple Branch Target Buffer
class BTB(
  val entries: Int
)(
  implicit p:  CoreParams)
    extends Module {
  val INDEX_BITS = log2Ceil(entries)

  val io = IO(new Bundle {
    val r = new BTBReadIO
    val w = new BTBWriteIO
  })

  val r_idx = io.r.pc(INDEX_BITS + 1, 2)
  val r_tag = io.r.pc(p.XLEN - 1, INDEX_BITS + 2)

  // storage
  val valid_storage  = RegInit(VecInit(Seq.fill(entries)(false.B)))
  val tag_storage    = Reg(Vec(entries, UInt((p.XLEN - (INDEX_BITS + 2)).W)))
  val target_storage = Reg(Vec(entries, UInt(p.XLEN.W)))
  val type_storage   = Reg(Vec(entries, UInt(BranchType.WIDTH)))

  // lookup
  val hit = valid_storage(r_idx) && tag_storage(r_idx) === r_tag
  io.r.valid  := hit
  io.r.target := target_storage(r_idx)
  io.r.btype  := type_storage(r_idx)

  // write
  val w_idx = io.w.pc(INDEX_BITS + 1, 2)
  val w_tag = io.w.pc(p.XLEN - 1, INDEX_BITS + 2)
  valid_storage(w_idx)  := Mux(io.w.en, true.B, valid_storage(w_idx))
  tag_storage(w_idx)    := Mux(io.w.en, w_tag, tag_storage(w_idx))
  target_storage(w_idx) := Mux(io.w.en, io.w.target, target_storage(w_idx))
  type_storage(w_idx)   := Mux(io.w.en, io.w.btype, type_storage(w_idx))
}

// Simple Return Address Stack
class RAS(
  entries:    Int
)(
  implicit p: CoreParams)
    extends Module {
  val io = IO(new Bundle {
    val push     = Input(Bool())
    val push_val = Input(UInt(p.XLEN.W))
    val pop      = Input(Bool())
    val peek     = Output(UInt(p.XLEN.W))
  })

  val stack = Reg(Vec(entries, UInt(p.XLEN.W)))
  val ptr   = RegInit(0.U(log2Ceil(entries).W))

  stack(ptr) := Mux(io.push, io.push_val, stack(ptr))

  ptr := MuxCase(
    ptr,
    Seq(
      (io.push && io.pop) -> ptr,
      io.push             -> (ptr + 1.U),
      io.pop              -> (ptr - 1.U)
    )
  )

  io.peek := stack(ptr - 1.U)
}

// Simple BPU (BTFN + RAS)
class BPU(
  implicit p: CoreParams)
    extends Module {
  val io = IO(new Bundle {
    val pc = Input(UInt(p.XLEN.W))

    val btb_w = new BTBWriteIO

    val predict_taken  = Output(Bool())
    val predict_target = Output(UInt(p.XLEN.W))
  })

  val btb = Module(new BTB(16))
  val ras = Module(new RAS(16))

  // BTB Update
  btb.io.w := io.btb_w

  // BTB Lookup
  btb.io.r.pc := io.pc
  val valid  = btb.io.r.valid
  val btype  = btb.io.r.btype
  val target = btb.io.r.target

  val is_call = valid && btype === BranchType.Call
  val is_ret  = valid && btype === BranchType.Ret

  ras.io.push     := is_call
  ras.io.push_val := io.pc + 4.U

  ras.io.pop := is_ret

  io.predict_taken := valid && (is_ret || is_call || btype === BranchType.Jump ||
    (btype === BranchType.Branch && target < io.pc))

  io.predict_target := Mux(is_ret, ras.io.peek, target)
}
