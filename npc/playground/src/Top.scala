import chisel3._

class InBundle extends Bundle {
  val btn = UInt(5.W)
  val sw = UInt(16.W)
  val ps2_clk = Bool()
  val ps2_data = Bool()
  val uart_rx = Bool()
}

class VGABundle extends Bundle {
  val CLK = Clock()
  val HSYNC = Bool()
  val VSYNC = Bool()
  val BLANK_N = Bool()
  val R = UInt(8.W)
  val G = UInt(8.W)
  val B = UInt(8.W)
}

class OutBundle extends Bundle {
  val uart_tx = Bool()
  val ledr = UInt(16.W)
  val vga = new VGABundle
  val seg = Vec(8, UInt(8.W))
}

class TopIO extends Bundle {
  val in = Input(new InBundle)
  val out = Output(new OutBundle)
}

class Top extends Module {
  val io = IO(new TopIO)

  io.out := 0.U.asTypeOf(io.out)
}
