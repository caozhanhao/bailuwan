// Copyright (c) 2025 caozhanhao
// SPDX-License-Identifier: MIT

// SDB is adapted from sdb in NEMU.

#include "sdb.hpp"
#include "dut_proxy.hpp"
#include "utils/disasm.hpp"

static void trace_and_difftest()
{
#ifdef CONFIG_ITRACE
    static Disassembler disasm;
    static bool inited = false;

    if (!inited)
    {
        inited = true;
        disasm.init();
    }
#endif

    auto& cpu = SIM.cpu();
    if (cpu.is_inst_ready_for_trace())
    {
#ifdef CONFIG_ITRACE
        auto str = disasm.disassemble(cpu.pc(), cpu.curr_inst());
        fprintf(stderr, FMT_WORD ": %s\n", cpu.pc(), str.c_str());
#endif

#ifdef CONFIG_FTRACE
        char buf[512];
        int ret = isa_ftrace_dump(buf, sizeof(buf));
        if (ret == 0)
            fprintf(stderr, "FTRACE: %s\n", buf);
#endif

#ifdef CONFIG_WP_BP
        // Before difftest
        wp_update();
        bp_update();
#endif
    }

    if (cpu.is_ready_for_difftest())
    {
        IFDEF(CONFIG_DIFFTEST, difftest_step());
    }
}

static void execute(uint64_t n)
{
    auto& cpu = SIM.cpu();
    while (n > 0)
    {
        SIM.single_cycle();
        if (SIM.has_got_ebreak())
        {
            SIM.dump_after_ebreak();

            sdb_state = SDBState::End;
            sdb_halt_ret = static_cast<int>(cpu.reg(10));
        }

        trace_and_difftest();

        if (sdb_state != SDBState::Running) break;

        if (cpu.is_inst_ready_for_trace())
            --n;

        if (sim_stop_requested)
        {
            sdb_state = SDBState::Stop;
            sim_stop_requested = 0;
            resume_from_ctrl_c = true;
            // SIM.cleanup();
            // exit(-1);
        }
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
