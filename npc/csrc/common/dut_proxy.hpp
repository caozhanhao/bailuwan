#ifndef COMMON_DUT_PROXY_HPP
#define COMMON_DUT_PROXY_HPP

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

class EBreakException : public std::exception
{
    int code;

public:
    explicit EBreakException(int code_) : code(code_)
    {
    }

    const char* what() const noexcept override
    {
        return "EBreakException";
    }

    int get_code() const { return code; }
};

#define CSR_TABLE                                                                                                      \
CSR_TABLE_ENTRY(mstatus, 0x300)                                                                                      \
CSR_TABLE_ENTRY(mtvec, 0x305)                                                                                        \
CSR_TABLE_ENTRY(mepc, 0x341)                                                                                         \
CSR_TABLE_ENTRY(mcause, 0x342)                                                                                       \
CSR_TABLE_ENTRY(mcycle, 0xb00)                                                                                       \
CSR_TABLE_ENTRY(mcycleh, 0xb80)                                                                                      \
CSR_TABLE_ENTRY(mvendorid, 0xf11)                                                                                    \
CSR_TABLE_ENTRY(marchid, 0xf12)

// Name to Index
// CSR_mstatus = 0x300, ...
enum CSR
{
#define CSR_TABLE_ENTRY(name, idx) CSR_##name = idx,
    CSR_TABLE
#undef CSR_TABLE_ENTRY
};

class CPUProxy
{
    uint32_t* pc_binding;
    uint32_t* dnpc_binding;
    uint32_t* inst_binding;
    uint8_t* difftest_ready;
    uint32_t* register_bindings[16];
    uint32_t* csr_bindings[4096];

public:
    CPUProxy() = default;

    void bind(TOP_NAME* dut);
    void dump_registers(std::ostream& os);
    uint32_t pc() const;
    uint32_t dnpc() const;
    uint32_t curr_inst() const;
    uint32_t reg(uint32_t idx) const;
    uint32_t csr(uint32_t idx) const;
    bool is_csr_valid(uint32_t idx) const;
    bool is_ready_for_difftest() const;
};

#define PMEM_LEFT  ((uint32_t)CONFIG_MBASE)
#define PMEM_RIGHT ((uint32_t)CONFIG_MBASE + CONFIG_MSIZE - 1)
#define RESET_VECTOR PMEM_LEFT

struct DUTMemory
{
    uint32_t* data{};
    size_t img_size{};

    void init(const std::string& filename);
    void destroy();

    uint32_t read(uint32_t raddr);
    void write(uint32_t waddr, uint32_t wdata, char wmask);

    bool in_pmem(uint32_t addr);
    uint8_t* guest_to_host(uint32_t paddr) const;
    uint32_t host_to_guest(uint8_t* haddr) const;
};

class SimHandle
{
    DUTMemory memory;
    CPUProxy cpu;
    uint64_t cycle_counter;
    uint64_t sim_time;
    uint32_t prev_inst;
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
    const auto& get_boot_time() const { return boot_time; }
};

extern TOP_NAME dut;
extern SimHandle sim_handle;
extern const char* csr_names[4096];
#endif
