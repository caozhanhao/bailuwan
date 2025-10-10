#include <VTop.h>
#include "macro.h"

#ifdef TRACE_FST
#include "verilated_fst_c.h"
#define TRACE_HANDLE_TYPE VerilatedFstC
#endif

#ifdef TRACE_VCD
#include "verilated_vcd_c.h"
#define TRACE_HANDLE_TYPE VerilatedVcdC
#endif

static TOP_NAME dut;
static TRACE_HANDLE *trace_handle;

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
  trace_handle = new TRACE_HANDLE_TYPE;
  Verilated::traceEverOn(true);
  dut->trace(trace_handle, 0);
  trace_handle->open(TRACE_FILENAME);

  reset(10);

  while(1) {
    single_cycle();
  }
}