// Copyright (c) 2025 caozhanhao
// SPDX-License-Identifier: MIT

package core

import chisel3._
import chisel3.util._
import bailuwan.CoreParams

class BTBReadIO(
  implicit p: CoreParams)
    extends Bundle {
  val pc = Input(UInt(p.XLEN.W))

  val valid     = Output(Bool())
  val target    = Output(UInt(p.XLEN.W))
  val is_uncond = Output(Bool())
}

class BTBWriteIO(
  implicit p: CoreParams)
    extends Bundle {
  val en        = Input(Bool())
  val pc        = Input(UInt(p.XLEN.W))
  val target    = Input(UInt(p.XLEN.W))
  val is_uncond = Input(Bool())
}

// Simple Branch Target Buffer
class BTB(
  val entries: Int = 16
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
  val valid_storage     = RegInit(VecInit(Seq.fill(entries)(false.B)))
  val tag_storage       = Reg(Vec(entries, UInt((p.XLEN - (INDEX_BITS + 2)).W)))
  val target_storage    = Reg(Vec(entries, UInt(p.XLEN.W)))
  val is_uncond_storage = RegInit(VecInit(Seq.fill(entries)(false.B)))

  // lookup
  val hit = valid_storage(r_idx) && tag_storage(r_idx) === r_tag
  io.r.valid     := hit
  io.r.target    := target_storage(r_idx)
  io.r.is_uncond := is_uncond_storage(r_idx)

  // write
  val w_idx = io.w.pc(INDEX_BITS + 1, 2)
  val w_tag = io.w.pc(p.XLEN - 1, INDEX_BITS + 2)
  valid_storage(w_idx)     := Mux(io.w.en, true.B, valid_storage(w_idx))
  tag_storage(w_idx)       := Mux(io.w.en, w_tag, tag_storage(w_idx))
  target_storage(w_idx)    := Mux(io.w.en, io.w.target, target_storage(w_idx))
  is_uncond_storage(w_idx) := Mux(io.w.en, io.w.is_uncond, is_uncond_storage(w_idx))
}

// Simple BPU (BTFN, Backward Taken, Forward Not-taken)
class BPU(
  implicit p: CoreParams)
    extends Module {
  val io = IO(new Bundle {
    val pc = Input(UInt(p.XLEN.W))

    val btb_valid     = Input(Bool())
    val btb_target    = Input(UInt(p.XLEN.W))
    val btb_is_uncond = Input(Bool())

    val predict_taken  = Output(Bool())
    val predict_target = Output(UInt(p.XLEN.W))
  })

  io.predict_taken  := io.btb_valid && (io.btb_is_uncond || io.btb_target < io.pc)
  io.predict_target := io.btb_target
}
