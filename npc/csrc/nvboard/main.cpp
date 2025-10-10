#include <nvboard.h>
#include <VTop.h>

static TOP_NAME dut;

void nvboard_bind_all_pins(TOP_NAME* top);


// 00000000 <_start>:
//      0:	01400513          	addi	a0,zero,20
//      4:	010000e7          	jalr	ra,16(zero) # 10 <fun>
//      8:	00c000e7          	jalr	ra,12(zero) # c <halt>
//
//   0000000c <halt>:
//      c:	00c00067          	jalr	zero,12(zero) # c <halt>
//
//   00000010 <fun>:
//     10:	00a50513          	addi	a0,a0,10
//     14:	00008067          	jalr	zero,0(ra)
uint32_t inst_mem[1024] = {
    0x01400513, // addi a0, zero, 20
    0x010000e7, // jalr ra, 16(zero)
    0x00c000e7, // jalr ra, 12(zero)
    0x00c000e7, // jalr	zero,12(zero) # c <halt>
    0x00a50513, // addi a0, a0, 10
    0x00008067, // jalr zero,0(ra)
};
static uint32_t pmem_read(uint32_t addr) {
  return inst_mem[addr / 4];
}

static void single_cycle() {
  dut.clock = 0; dut.eval();

  dut.io_mem_inst = pmem_read(dut.io_mem_pc);

  dut.clock = 1; dut.eval();
}

static void reset(int n) {
  dut.reset = 1;
  while (n -- > 0) single_cycle();
  dut.reset = 0;
}

int main() {
  nvboard_bind_all_pins(&dut);
  nvboard_init();

  reset(10);

  while(1) {
    nvboard_update();
    single_cycle();
  }
}
