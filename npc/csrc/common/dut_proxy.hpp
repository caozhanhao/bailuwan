#ifndef COMMON_H
#define COMMON_H

#include "VTop.h"
#include "utils/macro.hpp"

#ifdef TRACE_fst
#include "verilated_fst_c.h"
#define TFP_TYPE VerilatedFstC
#endif

#ifdef TRACE_vcd
#include "verilated_vcd_c.h"
#define TFP_TYPE VerilatedVcdC
#endif

class CPUProxy
{
    uint32_t* pc_binding;
    uint32_t* inst_binding;
    uint32_t* register_bindings[16];

public:
    CPUProxy() = default;

    void bind(TOP_NAME* dut);
    void dump_registers(std::ostream& os);
    uint32_t pc();
    uint32_t curr_inst();
    uint32_t reg(uint32_t idx);
};

// Byte * 1024 * 1024 * 512 Byte -> 512 MB
#define DUT_MEMORY_MAXSIZE (1024 * 1024 * 512)
#define IO_SPACE_MAX (32 * 1024 * 1024)
struct DUTMemory
{
    uint32_t* data{};
    size_t size{};

    void init(const std::string& filename);
    void destroy();
};

class SimHandle
{
    DUTMemory memory;
    CPUProxy cpu;
    uint64_t cycle_counter;
    uint64_t sim_time;
    IFDEF(TRACE, TFP_TYPE* tfp);
    std::chrono::high_resolution_clock::time_point boot_time;

    void init_trace();
    void cleanup_trace();

public:
    SimHandle() = default;

    void init_sim(const std::string& filename);
    void cleanup();
    void single_cycle();
    void reset(int n);

    uint64_t get_cycles() const { return cycle_counter; }
    CPUProxy& get_cpu() { return cpu; }
    DUTMemory& get_memory() { return memory; }
    const auto& get_boot_time() const  { return boot_time; }
};

extern TOP_NAME dut;
extern SimHandle sim_handle;
#endif
