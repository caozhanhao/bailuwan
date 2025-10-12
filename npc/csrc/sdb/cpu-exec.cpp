#include <VTop.h>

#include "dut_proxy.hpp"
#include "sdb.hpp"
#include "utils/disasm.hpp"

static void execute(uint64_t n)
{
    for (; n > 0; n--)
    {
        try
        {
            sim_handle.single_cycle();
        }
        catch (EBreakException& e)
        {
            sdb_state = SDBState::End;
        }
        if (sdb_state != SDBState::Running) break;
    }
}

void cpu_exec(uint64_t n)
{
    switch (sdb_state)
    {
    case SDBState::End:
    case SDBState::Abort:
    case SDBState::Quit:
        printf("Program execution has ended. To restart the program, exit NEMU and run again.\n");
        return;
    default: sdb_state = SDBState::Running;
    }

    execute(n);

    if (sdb_state == SDBState::Running)
        sdb_state = SDBState::Stop;
}
