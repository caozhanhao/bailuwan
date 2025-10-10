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

static void single_cycle() {
  dut.clock = 0; dut.eval();
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

  while(1) {
    single_cycle();
  }
}