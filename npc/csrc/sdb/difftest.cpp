#include <dlfcn.h>

#include "dut_proxy.hpp"
#include "sdb.hpp"

enum { DIFFTEST_TO_DUT, DIFFTEST_TO_REF };

using difftest_memcpy_t = void (*)(uint32_t addr, void* buf, size_t n, bool direction);
using difftest_regcpy_t = void (*)(void* dut, bool direction);
using difftest_exec_t = void (*)(uint64_t n);
using difftest_raise_intr_t = void (*)(uint64_t NO);

difftest_memcpy_t ref_difftest_memcpy;
difftest_regcpy_t ref_difftest_regcpy;
difftest_exec_t ref_difftest_exec;
difftest_raise_intr_t ref_difftest_raise_intr;

struct diff_context_t
{
    word_t gpr[32];
    word_t pc;
};

#ifdef CONFIG_DIFFTEST

void init_difftest(size_t img_size)
{
    const char* ref_so_file = "csrc/common/lib/riscv32-nemu-interpreter-so";

    void* handle;
    handle = dlopen(ref_so_file, RTLD_LAZY);
    assert(handle);

    ref_difftest_memcpy = reinterpret_cast<difftest_memcpy_t>(dlsym(handle, "difftest_memcpy"));
    assert(ref_difftest_memcpy);

    ref_difftest_regcpy = reinterpret_cast<difftest_regcpy_t>(dlsym(handle, "difftest_regcpy"));
    assert(ref_difftest_regcpy);

    ref_difftest_exec = reinterpret_cast<difftest_exec_t>(dlsym(handle, "difftest_exec"));
    assert(ref_difftest_exec);

    ref_difftest_raise_intr = reinterpret_cast<difftest_raise_intr_t>(dlsym(handle, "difftest_raise_intr"));
    assert(ref_difftest_raise_intr);

    using difftest_init_t = void (*)(int);
    auto ref_difftest_init = reinterpret_cast<difftest_init_t>(dlsym(handle, "difftest_init"));
    assert(ref_difftest_init);

    Log("Differential testing: %s", ANSI_FMT("ON", ANSI_FG_GREEN));
    Log("The result of every instruction will be compared with %s. "
        "This will help you a lot for debugging, but also significantly reduce the performance. "
        "If it is not necessary, you can turn it off in menuconfig.", ref_so_file);

    ref_difftest_init(0);
    auto& mem = sim_handle.get_memory();
    auto& cpu = sim_handle.get_cpu();

    ref_difftest_memcpy(RESET_VECTOR, mem.guest_to_host(RESET_VECTOR), img_size, DIFFTEST_TO_REF);

    diff_context_t ctx;
    for (int i = 0; i < 16; i++)
        ctx.gpr[i] = cpu.reg(i);
    ctx.pc = cpu.pc();

    ref_difftest_regcpy(&ctx, DIFFTEST_TO_REF);
}

static void checkregs(diff_context_t* ref)
{
    auto& cpu = sim_handle.get_cpu();
    bool match = true;
    for (int i = 0; i < 16; i++)
    {
        if (cpu.reg(i) != ref->gpr[i])
        {
            Log("reg: x%d, expected " FMT_WORD ", but got " FMT_WORD "\n", i, ref->gpr[i], cpu.reg(i));
            match = false;
        }
    }

    if (cpu.pc() != ref->pc)
    {
        Log("pc: expected " FMT_WORD ", but got " FMT_WORD "\n", ref->pc, cpu.pc());
        match = false;
    }

    if (!match)
    {
        sdb_state = SDBState::Abort;
        isa_reg_display();
    }
}

void difftest_step()
{
    diff_context_t ref_r;

    ref_difftest_exec(1);
    ref_difftest_regcpy(&ref_r, DIFFTEST_TO_DUT);

    checkregs(&ref_r);
}
#else
void init_difftest()
{
}
void difftest_step()
{
}
#endif
