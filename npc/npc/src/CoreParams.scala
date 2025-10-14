package top

case class CoreParams() {
  val XLEN: Int = 32
  val RegCount: Int = 16
  val ResetVector: Int = 0x80000000
}
