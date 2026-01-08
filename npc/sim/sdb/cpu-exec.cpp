/***************************************************************************************
 * Copyright (c) 2014-2024 Zihao Yu, Nanjing University
 *
 * NEMU is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan
 *PSL v2. You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY
 *KIND, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
 *NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 *
 * See the Mulan PSL v2 for more details.
 ***************************************************************************************/

#include "sdb.hpp"
#include "dut_proxy.hpp"
#include "utils/disasm.hpp"

static void trace_and_difftest()
{
    // Trace ready is asserted in EXU, thus operations in trace should
    // use exu_pc, exu_inst, ...
    auto& cpu = SIM.cpu();
    if (cpu.exu_inst_trace_ready())
    {
#ifdef CONFIG_ITRACE
        auto str = rv32_disasm(cpu.exu_pc(), cpu.exu_inst());
        fprintf(stderr, FMT_WORD ": %s\n", cpu.exu_pc(), str.c_str());
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

    // Difftest ready is asserted in WBU, and operations in trace should
    // use difftest_pc, difftest_inst, ...
    if (cpu.difftest_ready())
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

        if (cpu.exu_inst_trace_ready())
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
