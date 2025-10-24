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
}

class AddressChannel(
  implicit p: AXIProperty)
    extends Bundle {
  val addr = UInt(p.ADDR_WIDTH.W)
  val prot = UInt(3.W)
}

class WriteDataChannel(
  implicit p: AXIProperty)
    extends Bundle {
  val data = UInt(32.W)
  val strb = UInt(4.W)
}

class WriteResponseChannel(
  implicit p: AXIProperty)
    extends Bundle {
  val resp = UInt(p.BRESP_WIDTH.W)
}

class ReadDataChannel(
  implicit p: AXIProperty)
    extends Bundle {
  val data = UInt(32.W)
  val resp = UInt(p.RRESP_WIDTH.W)
}

class AXI4Lite(
  implicit p: AXIProperty)
    extends Bundle {
  val aw = Decoupled(new AddressChannel)
  val w  = Decoupled(new WriteDataChannel)
  val b  = Flipped(Decoupled(new WriteResponseChannel))
  val ar = Decoupled(new AddressChannel)
  val r  = Flipped(Decoupled(new ReadDataChannel))
}
