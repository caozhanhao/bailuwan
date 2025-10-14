package top

case class CoreParams() {
  val XLEN: Int = 64
  val RegCount: Int = 16
  val ResetVector: Int = 0x80000000
}
