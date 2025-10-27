// https://github.com/OSCPU/ysyxSoC/blob/ysyx6/spec/cpu-interface.md
package amba

import chisel3._
import chisel3.util._

object AXIResp {
  // Normal access success.
  // Indicates that a normal access has been successful.
  // Can also indicate that an exclusive access has failed.
  val OKAY = 0.U

  // Exclusive access okay.
  // Indicates that either the read or write portion of an exclusive access has been successful.
  val EXOKAY = 1.U

  // Slave error.
  // Used when the access has reached the slave successfully, but the slave wishes to return
  // an error condition to the originating master.
  val SLVERR = 2.U

  // Decode error.
  // Generated, typically by an interconnect component, to indicate that there is no slave
  // at the transaction address.
  val DECERR = 3.U
}

case class AXIProperty() {
  val ADDR_WIDTH  = 32
  val BRESP_WIDTH = 2
  val RRESP_WIDTH = 2
  val DATA_WIDTH  = 32
  val ID_W_WIDTH  = 4
  val ID_R_WIDTH  = 4
}

class WriteAddressChannel(
  implicit p: AXIProperty)
    extends Bundle {
  val addr  = UInt(p.ADDR_WIDTH.W)
  val id    = UInt(p.ID_W_WIDTH.W)
  val len   = UInt(8.W)
  val size  = UInt(3.W)
  val burst = UInt(2.W)
}

class WriteDataChannel(
  implicit p: AXIProperty)
    extends Bundle {
  val data = UInt(p.DATA_WIDTH.W)
  val strb = UInt((p.DATA_WIDTH / 8).W)
  val last = Bool()
}

class WriteResponseChannel(
  implicit p: AXIProperty)
    extends Bundle {
  val resp = UInt(p.BRESP_WIDTH.W)
  val id   = UInt(p.ID_W_WIDTH.W)
}

class ReadAddressChannel(
  implicit p: AXIProperty)
    extends Bundle {
  val addr  = UInt(p.ADDR_WIDTH.W)
  val id    = UInt(p.ID_R_WIDTH.W)
  val len   = UInt(8.W)
  val size  = UInt(3.W)
  val burst = UInt(2.W)
}

class ReadDataChannel(
  implicit p: AXIProperty)
    extends Bundle {
  val data = UInt(p.DATA_WIDTH.W)
  val resp = UInt(p.RRESP_WIDTH.W)
  val last = Bool()
  val id   = UInt(p.ID_R_WIDTH.W)
}

class AXI4(
  implicit p: AXIProperty)
    extends Bundle {
  val aw = Decoupled(new WriteAddressChannel)
  val w  = Decoupled(new WriteDataChannel)
  val b  = Flipped(Decoupled(new WriteResponseChannel))
  val ar = Decoupled(new ReadAddressChannel)
  val r  = Flipped(Decoupled(new ReadDataChannel))
}
