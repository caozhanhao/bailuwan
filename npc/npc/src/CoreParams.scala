package top

case class CoreParams() {
  val XLEN: Int = 32
  val RegCount: Int = 16
  val RESET_VECTOR: Int = 0x80000000
}
