#include <VTop.h>

#ifdef TRACE_FST
#include "verilated_fst_c.h"
#define TRACE_HANDLE_TYPE VerilatedFstC
#endif

#ifdef TRACE_VCD
#include "verilated_vcd_c.h"
#define TRACE_HANDLE_TYPE VerilatedVcdC
#endif

#ifdef TRACE
static TRACE_HANDLE_TYPE *trace_handle;
#endif

static TOP_NAME dut;



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

#define STRINGIFY(s)        #s
#define TOSTRING(s)         STRINGIFY(s)

int main() {
#ifdef TRACE
  trace_handle = new TRACE_HANDLE_TYPE;
  Verilated::traceEverOn(true);
  dut.trace(trace_handle, 0);
  trace_handle->open(TOSTRING(TRACE_FILENAME));
#endif

  reset(10);

  uint64_t time = 0;
  while(1) {
    single_cycle();
    trace_handle->dump(time++);
  }

#ifdef TRACE
  trace_handle->close();
  delete trace_handle;
#endif
  return 0;
}