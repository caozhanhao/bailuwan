package top

import chisel3._

class InBundle extends Bundle {
  val btn      = UInt(5.W)
  val sw       = UInt(16.W)
  val ps2_clk  = Bool()
  val ps2_data = Bool()
  val uart_rx  = Bool()
}

class VGABundle extends Bundle {
  val CLK     = Clock()
  val HSYNC   = Bool()
  val VSYNC   = Bool()
  val BLANK_N = Bool()
  val R       = UInt(8.W)
  val G       = UInt(8.W)
  val B       = UInt(8.W)
}

class OutBundle extends Bundle {
  val uart_tx = Bool()
  val ledr    = UInt(16.W)
  val vga     = new VGABundle
  val seg     = Vec(8, UInt(8.W))
}

class TopIO extends Bundle {
  val in  = Input(new InBundle)
  val out = Output(new OutBundle)
}

class Top extends Module {
  val io = IO(new TopIO)

  io.out := 0.U.asTypeOf(io.out)

  val code =
    """
      |li r0, 10        # load imm 10 for loop bound
      |li r1, 0         # i = 0
      |li r2, 0         # sum = 0
      |li r3, 1         # load step imm 1 (step -> r3)
      |
      |loop:
      |add r1, r1, r3   # i += 1
      |add r2, r2, r1   # sum += i
      |bner0 r1, loop   # branch to %4 if i != 10
      |out r2           # set output as sum
      |
      |halt:
      |bner0 r3, halt   # b 2 != 11, %7 (dead loop)
      |""".stripMargin

  val program = sCPU.assemble(code)
  val cpu = Module(new sCPU(program))

  val tbl = Seq(
    0x7E,
    0x30,
    0x6D,
    0x79,
    0x33,
    0x5B,
    0x5F,
    0x70,
    0x7F,
    0x7B,
    0x77,
    0x1F,
    0x4E,
    0x3D,
    0x4F,
    0x47,
  )

  val data = RegInit(0.U(8.W))
  val patterns = VecInit(tbl.map((x: Int) => ~(x.U(8.W) << 1)))
  io.out.seg(0) := patterns(data(3, 0))
  io.out.seg(1) := patterns(data(7, 4))

  when (cpu.io.outValid) {
    data := cpu.io.out
  }
}
