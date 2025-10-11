#include <VTop.h>

#ifdef TRACE_fst
#include "verilated_fst_c.h"
#define TFP_TYPE VerilatedFstC
#endif

#ifdef TRACE_vcd
#include "verilated_vcd_c.h"
#define TFP_TYPE VerilatedVcdC
#endif

#ifdef TRACE
static TFP_TYPE *tfp;
#endif

static TOP_NAME dut;


bool running = true;

extern "C" {
uint32_t memory[65536];

void ebreak_handler() {
  printf("ebreak\n");
  running = false;
}

extern "C" int pmem_read(int raddr) {
  return memory[((unsigned)raddr & ~0x3u) / 4];
}
extern "C" void pmem_write(int waddr, int wdata, char wmask) {
    unsigned idx = ((unsigned)waddr & ~0x3u) / 4u;

    uint32_t cur = memory[idx];
    uint32_t newv = cur;
    uint32_t wd = (uint32_t)wdata;
    uint8_t mask = (uint8_t)wmask;

    for (int i = 0; i < 4; ++i) {
        if (mask & (1u << i)) {
            uint32_t byte_mask = 0xFFu << (i * 8);
            uint32_t src_byte = (wd >> (i * 8)) & 0xFFu;
            newv = (newv & ~byte_mask) | (src_byte << (i * 8));
        }
    }

    memory[idx] = newv;
}
}

// 00000000 <_start>:
//      0:	01400513          	addi	a0,zero,20
//      4:	010000e7          	jalr	ra,16(zero) # 10 <fun>
//      8:	00c000e7          	jalr	ra,12(zero) # c <halt>
//
//   0000000c <halt>:
//      c:  00100073            ebreak
//
//   00000010 <fun>:
//     10:	00a50513          	addi	a0,a0,10
//     14:	00008067          	jalr	zero,0(ra)
uint32_t inst_mem[1024] = {
    0x01400513, // addi a0, zero, 20
    0x010000e7, // jalr ra, 16(zero)
    0x00c000e7, // jalr ra, 12(zero)
    0x00100073, // ebreak
    0x00a50513, // addi a0, a0, 10
    0x00008067, // jalr zero,0(ra)
};

static uint32_t pmem_read(uint32_t addr) {
  return inst_mem[addr / 4];
}
static uint64_t sim_time = 0;

static void single_cycle() {
#ifdef TRACE
  tfp->dump(sim_time++);
#endif

  dut.clock = 0;
  dut.io_mem_inst = pmem_read(dut.io_mem_pc);
  dut.eval();

#ifdef TRACE
  tfp->dump(sim_time++);
#endif

  dut.clock = 1; dut.eval();
}

static void reset(int n) {
  dut.reset = 1;
  while (n -- > 0) single_cycle();
  dut.reset = 0;
}

#define STRINGIFY(s)        #s
#define TOSTRING(s)         STRINGIFY(s)

int main(int argc, char* argv[]) {
  if (argc != 2) {
    printf("Usage: %s [number of cycles]\n", argv[0]);
    return -1;
  }

  int cycles = atoi(argv[1]);

#ifdef TRACE
  tfp = new TFP_TYPE;
  Verilated::traceEverOn(true);
  dut.trace(tfp, 0);
  tfp->open(TOSTRING(TRACE_FILENAME));
#endif

  reset(10);

  while(running && cycles-- > 0) {
    single_cycle();
  }

#ifdef TRACE
  tfp->close();
  delete tfp;
#endif
  return 0;
}