#include <iostream>
#include <cstdint>
#include <cassert>
#include <cstring>
#include <dlfcn.h>
#include <functional>

#include "icachesim.hpp"

constexpr auto RESET_VECTOR = 0x30000000;
constexpr auto MAX_IMAGE_SIZE = 32 * 1024 * 1024;
constexpr auto BATCH_SIZE = 8192;

enum { DIFFTEST_TO_DUT, DIFFTEST_TO_REF };

using difftest_memcpy_t = void (*)(uint32_t addr, void* buf, size_t n, bool direction);
using difftest_regcpy_t = void (*)(void* dut, bool direction);
using difftest_exec_t = void (*)(uint64_t n);
using difftest_raise_intr_t = void (*)(uint64_t NO);

using difftest_cachesim_init_t = void (*)(uint32_t batch_size);
using difftest_cachesim_step_t = void (*)(void* batch);

difftest_memcpy_t ref_difftest_memcpy;
difftest_regcpy_t ref_difftest_regcpy;
difftest_exec_t ref_difftest_exec;
difftest_raise_intr_t ref_difftest_raise_intr;

difftest_cachesim_init_t ref_difftest_cachesim_init;
difftest_cachesim_step_t ref_difftest_cachesim_step;

struct diff_context_t
{
    uint32_t gpr[32];
    uint32_t pc;
    uint32_t csr[4096];
};

void init(void* img, size_t img_size)
{
    const char* ref_so_file = "sim/common/lib/riscv32-nemu-interpreter-so";

    auto handle = dlopen(ref_so_file, RTLD_LAZY);
    assert(handle);

    ref_difftest_memcpy = reinterpret_cast<difftest_memcpy_t>(dlsym(handle, "difftest_memcpy"));
    assert(ref_difftest_memcpy);

    ref_difftest_regcpy = reinterpret_cast<difftest_regcpy_t>(dlsym(handle, "difftest_regcpy"));
    assert(ref_difftest_regcpy);

    ref_difftest_exec = reinterpret_cast<difftest_exec_t>(dlsym(handle, "difftest_exec"));
    assert(ref_difftest_exec);

    ref_difftest_raise_intr = reinterpret_cast<difftest_raise_intr_t>(dlsym(handle, "difftest_raise_intr"));
    assert(ref_difftest_raise_intr);

    ref_difftest_cachesim_init = reinterpret_cast<difftest_cachesim_init_t>(dlsym(handle, "difftest_cachesim_init"));
    assert(ref_difftest_cachesim_init);

    ref_difftest_cachesim_step = reinterpret_cast<difftest_cachesim_step_t>(dlsym(handle, "difftest_cachesim_step"));
    assert(ref_difftest_cachesim_step);

    using difftest_init_t = void (*)(int);
    auto ref_difftest_init = reinterpret_cast<difftest_init_t>(dlsym(handle, "difftest_init"));
    assert(ref_difftest_init);

    // Initialize DiffTest
    ref_difftest_init(0);

    // Initialize Memory
    ref_difftest_memcpy(RESET_VECTOR, img, img_size, DIFFTEST_TO_REF);

    // Initialize PC
    diff_context_t ctx{};
    ctx.pc = RESET_VECTOR;
    ref_difftest_regcpy(&ctx, DIFFTEST_TO_REF);

    // Initialize cachesim
    ref_difftest_cachesim_init(BATCH_SIZE);
}

struct cachesim_batch
{
    // PC stream
    uint32_t* data;
    uint32_t size;
};

uint32_t buffer[BATCH_SIZE];

void drain_pc_stream(const std::function<void(uint32_t)>& func)
{
    cachesim_batch batch{};
    batch.data = buffer;

    while (true)
    {
        ref_difftest_cachesim_step(&batch);

        for (uint32_t i = 0; i < batch.size; i++)
            func(buffer[i]);

        if (batch.size != BATCH_SIZE)
            break;
    }
}

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: cachesim image_path\n");
        return -1;
    }

    // 64 MB
    auto image = malloc(MAX_IMAGE_SIZE);
    memset(image, 0, MAX_IMAGE_SIZE);

    printf("Initializing from %s\n", argv[1]);
    FILE* fp = fopen(argv[1], "rb");

    auto bytes_read = fread(image, 1, MAX_IMAGE_SIZE, fp);

    init(image, bytes_read);

    drain_pc_stream([](uint32_t pc)
    {
        printf("0x%x\n", pc);
    });
    //
    // ICacheSim sim(64, 4, 1);
    // drain_pc_stream([&sim](uint32_t pc) { sim.step(pc); });
    // sim.dump(stdout);

    free(image);
    return 0;
}
