#include "trace.hpp"
#include "dut_proxy.hpp"

TFP_TYPE* tfp;

void trace_init()
{
#ifdef TRACE
    tfp = new TFP_TYPE;
    Verilated::traceEverOn(true);
    dut.trace(tfp, 0);
    tfp->open(TOSTRING(TRACE_FILENAME));
#endif
}

void trace_cleanup()
{
#ifdef TRACE
    tfp->close();
    delete tfp;
#endif
}
