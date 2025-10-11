#include <VTop.h>

#include "trace.h"
#include "macro.h"
#include "common.h"

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

static uint64_t sim_time = 0;

static void single_cycle()
{
    IFDEF(TRACE, tfp->dump(sim_time++));

    dut.clock = 0;
    dut.eval();

    IFDEF(TRACE, tfp->dump(sim_time++));

    dut.clock = 1;
    dut.eval();
}

void reset(int n)
{
    dut.reset = 1;
    while (n-- > 0)
        single_cycle();
    dut.reset = 0;
}

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        printf("Usage: %s [number of cycles]\n", argv[0]);
        return -1;
    }

    int cycles = atoi(argv[1]);

    trace_init();

    reset(10);

    while (cycles-- > 0)
    {
        single_cycle();
    }

    trace_cleanup();
    return 0;
}
