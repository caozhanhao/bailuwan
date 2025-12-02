// Copyright (c) 2025 caozhanhao
// SPDX-License-Identifier: MIT

#ifndef BAILUWAN_COMMON_DUT_PROXY_HPP
#define BAILUWAN_COMMON_DUT_PROXY_HPP

#include "utils/macro.hpp"
#include "config.hpp"

#include TOSTRING(TOP_NAME.h)

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

    [[nodiscard]] const char* what() const noexcept override
    {
        return "EBreakException";
    }

    [[nodiscard]] int get_code() const { return code; }
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

#define PERF_COUNTER_TABLE \
PERF_COUNTER_TABLE_ENTRY(ifu_fetched) \
PERF_COUNTER_TABLE_ENTRY(lsu_read) \
PERF_COUNTER_TABLE_ENTRY(exu_done) \
PERF_COUNTER_TABLE_ENTRY(alu_ops) \
PERF_COUNTER_TABLE_ENTRY(br_ops) \
PERF_COUNTER_TABLE_ENTRY(lsu_ops) \
PERF_COUNTER_TABLE_ENTRY(csr_ops) \
PERF_COUNTER_TABLE_ENTRY(other_ops) \
PERF_COUNTER_TABLE_ENTRY(all_ops) \
PERF_COUNTER_TABLE_ENTRY(alu_cycles) \
PERF_COUNTER_TABLE_ENTRY(br_cycles) \
PERF_COUNTER_TABLE_ENTRY(lsu_cycles) \
PERF_COUNTER_TABLE_ENTRY(csr_cycles) \
PERF_COUNTER_TABLE_ENTRY(other_cycles) \
PERF_COUNTER_TABLE_ENTRY(wait_cycles) \
PERF_COUNTER_TABLE_ENTRY(all_cycles)

class CPUProxy
{
    friend class SimHandle;

    struct Bindings
    {
        // Registers
        uint32_t* gprs[16];
        uint32_t* csrs[4096];

        // Normal Signals
        uint32_t* pc;
        uint32_t* dnpc;
        uint32_t* inst;
        uint8_t* difftest_ready;
        uint8_t* ifu_state;

        struct PerfCounters
        {
            // Perf Counters
#define PERF_COUNTER_TABLE_ENTRY(name) uint64_t* name;
            PERF_COUNTER_TABLE
#undef PERF_COUNTER_TABLE_ENTRY
        } perf_counters;
    } bindings;

public:
    CPUProxy() = default;

    void bind(const TOP_NAME* dut);
    void dump_gprs(FILE* stream = stderr) const;
    void dump_csrs(FILE* stream = stderr) const;
    void dump_perf_counters(FILE* stream = stderr);
    void dump(FILE* stream = stderr);
    [[nodiscard]] uint32_t pc() const;
    [[nodiscard]] uint32_t dnpc() const;
    [[nodiscard]] uint32_t curr_inst() const;
    [[nodiscard]] uint64_t inst_count() const;
    [[nodiscard]] uint64_t cycle_count() const;
    [[nodiscard]] uint32_t reg(uint32_t idx) const;
    [[nodiscard]] uint32_t csr(uint32_t idx) const;
    [[nodiscard]] bool is_csr_valid(uint32_t idx) const;
    [[nodiscard]] bool is_ready_for_difftest() const;
    [[nodiscard]] bool is_inst_valid() const;
};

struct DUTMemory
{
    uint8_t* flash_data{};
    uint8_t* psram_data{};
    uint8_t* sdram_data{};
    uint8_t* mrom_data{};

    size_t inst_memory_size{};

    void init(const std::string& filename);
    void destroy();

    template <size_t S>
    static uint32_t align_down(uint32_t addr)
    {
        return addr & ~(S - 1);
    }

    [[noreturn]] static void out_of_bound_abort(uint32_t addr);

    template <typename T>
    T read(uint32_t uaddr)
    {
        // // Clock
        // if (uaddr - RTC_MMIO >= 8 && uaddr - RTC_MMIO <= 28)
        // {
        //     std::time_t t = std::time(nullptr);
        //     std::tm* now = std::gmtime(&t);
        //     switch (uaddr - RTC_MMIO)
        //     {
        //     case 8:
        //         return now->tm_sec;
        //     case 12:
        //         return now->tm_min;
        //     case 16:
        //         return now->tm_hour;
        //     case 20:
        //         return now->tm_mday;
        //     case 24:
        //         return now->tm_mon + 1;
        //     case 28:
        //         return now->tm_year + 1900;
        //     default: assert(false);
        //     }
        //     assert(false);
        // }

        // Memory
        if (!in_sim_mem(uaddr))
            out_of_bound_abort(uaddr);

        return *reinterpret_cast<T*>(guest_to_host(uaddr));
    }

    template <typename T>
    void write(uint32_t uaddr, T wdata, uint8_t wmask)
    {
        if (!in_sim_mem(uaddr))
            out_of_bound_abort(uaddr);

        if (wmask == 0)
            return;

        auto haddr = guest_to_host(uaddr);
        auto* u8data = reinterpret_cast<uint8_t*>(&wdata);
        for (int i = 0; i < sizeof(T); i++)
        {
            if (wmask & (1 << i))
                haddr[i] = u8data[i];
        }
    }

    static bool in_mrom(uint32_t addr);
    static bool in_sram(uint32_t addr);
    static bool in_flash(uint32_t addr);
    static bool in_psram(uint32_t addr);
    static bool in_sdram(uint32_t addr);
    static bool in_device(uint32_t addr);
    static bool in_sim_mem(uint32_t addr);
    [[nodiscard]] uint8_t* guest_to_host(uint32_t paddr) const;
    [[nodiscard]] uint32_t host_to_guest(uint8_t* haddr) const;
};

class SimHandle
{
    TOP_NAME* dut{};
    DUTMemory memory;
    CPUProxy cpu_proxy{};
    uint64_t cycle_counter{};
    uint64_t sim_time{};
    uint32_t prev_inst{};
    IFDEF(TRACE, TFP_TYPE* tfp{});
    std::chrono::high_resolution_clock::time_point boot_timepoint;
    std::string img_path;
    std::string statistics_path;

    void init_trace();
    void cleanup_trace();

#ifdef BAILUWAN_SIM_MODE
    static constexpr auto mode = TOSTRING(BAILUWAN_SIM_MODE);
#else
#error "Unknown simulation mode"
#endif

    static constexpr auto trace_mode =
#if !defined(TRACE)
        "disabled"
#elif defined(TRACE_fst)
        "fst"
#elif defined(TRACE_vcd)
    "vcd"
#else
#error "Unknown trace"
#endif
    ;

public:
    SimHandle() = default;

    void init_sim(TOP_NAME* dut_, const char* img_path_, const char* statistics_path_);
    void cleanup();
    void single_cycle();
    void reset(int n);

    void dump_statistics(FILE* stream = stderr) const;
    void dump_statistics_json(FILE* stream = nullptr) const;

    [[nodiscard]] uint64_t simulator_cycles() const { return cycle_counter; }
    CPUProxy& cpu() { return cpu_proxy; }
    [[nodiscard]] const CPUProxy& cpu() const { return cpu_proxy; }
    DUTMemory& mem() { return memory; }
    [[nodiscard]] const DUTMemory& mem() const { return memory; }
    [[nodiscard]] const auto& boot_tp() const { return boot_timepoint; }

    [[nodiscard]] uint64_t elapsed_time() const
    {
        return std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::high_resolution_clock::now() - boot_timepoint)
            .count();
    }
};

extern TOP_NAME DUT;
extern SimHandle SIM;
extern const char* csr_names[4096];
extern const char* gpr_names[32];
#endif
