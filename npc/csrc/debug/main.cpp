#include <VTop.h>

static TOP_NAME dut;

static void single_cycle() {
  dut.clock = 0; dut.eval();
  dut.clock = 1; dut.eval();
}

static void reset(int n) {
  dut.reset = 1;
  while (n -- > 0) single_cycle();
  dut.reset = 0;
}

int main() {
  reset(10);

  while(1) {
    nvboard_update();
    single_cycle();
  }
}
