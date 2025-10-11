#ifndef NPC_CSRC_TRACE_HPP
#define NPC_CSRC_TRACE_HPP

#include "macro.hpp"

#ifdef TRACE_fst
#include "verilated_fst_c.h"
#define TFP_TYPE VerilatedFstC
#endif

#ifdef TRACE_vcd
#include "verilated_vcd_c.h"
#define TFP_TYPE VerilatedVcdC
#endif

#ifdef TRACE
extern TFP_TYPE* tfp;
#endif

void trace_init();
void trace_cleanup();

#endif

