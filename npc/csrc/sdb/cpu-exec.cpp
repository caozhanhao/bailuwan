#include <VTop.h>

#include "sdb.hpp"
#include "dut_proxy.hpp"
#include "utils/disasm.hpp"

static void trace_and_difftest() {
    static Disassembler disasm;
    static bool inited = false;
    if (!inited)
        disasm.init();

    auto& cpu = sim_handle.get_cpu();
    auto str = disasm.disassemble(cpu.pc(), cpu.curr_inst());
#define CONFIG_ITRACE
    IFDEF(CONFIG_ITRACE, Log("%s", str.c_str()));

#ifdef CONFIG_FTRACE
    char buf[256];
    int ret = isa_ftrace_dump(_this, buf, sizeof(buf));
    if (ret == 0) {
        log_write("FTRACE: %s\n", buf);
        if (g_print_step)
            printf("FTRACE: %s\n", buf);
    }
#endif

    IFDEF(CONFIG_DIFFTEST, difftest_step(_this->pc, dnpc));

    wp_update();
}

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

        trace_and_difftest();

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
