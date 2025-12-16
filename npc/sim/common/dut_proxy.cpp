// Copyright (c) 2025 caozhanhao
// SPDX-License-Identifier: MulanPSL-2.0

#include "config.hpp"
#include "dut_proxy.hpp"

#include <verilated.h>
#include <verilated_syms.h>
#include <getopt.h>

#include <iostream>
#include <iomanip>
#include <type_traits>
#include <variant>

TOP_NAME DUT;
SimHandle SIM;
const char* csr_names[4096];
const char* gpr_names[32] = {
    "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2", "s0", "s1", "a0",
    "a1", "a2", "a3", "a4", "a5", "a6", "a7", "s2", "s3", "s4", "s5",
    "s6", "s7", "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
};

void CPUProxy::bind(const TOP_NAME* this_dut)
{
    auto scope_map = this_dut->contextp()->scopeNameMap();
    assert(scope_map && "scopeNameMap() == nullptr");

    std::map<std::string, void*> var_maps;
    for (auto& [scope_name, scope] : *scope_map)
    {
        if (auto varsp = scope->varsp())
        {
            for (const auto& [name, var] : *varsp)
            {
                if (var_maps.count(name))
                {
                    fprintf(stderr, "CPUProxy::bind: Multiple signals named %s\n", name);
                    assert(false);
                }
                var_maps[name] = var.datap();
            }
        }
    }

    auto find_signal = [&](const std::string& target) -> void*
    {
        auto exposed_name = "exposed_signal_" + target;
        auto it = var_maps.find(exposed_name);
        if (it != var_maps.end())
            return it->second;
        assert(false && "Can not find signal");
        return nullptr;
    };

#define BIND_SIGNAL(name, signal_name) bindings.name = static_cast<std::remove_reference_t<decltype(bindings.name)>>(find_signal(signal_name));

    // GPRs
    auto verilated_regs = static_cast<uint32_t*>(find_signal("regs"));
    for (size_t i = 0; i < 16; ++i)
        bindings.gprs[i] = &verilated_regs[i];

    // CSRs
#define CSR_TABLE_ENTRY(name, idx) BIND_SIGNAL(csrs[idx], TOSTRING(name))
    CSR_TABLE
#undef CSR_TABLE_ENTRY

    // Normal Signals
#define SIGNAL_TABLE_ENTRY(type, name) BIND_SIGNAL(name, TOSTRING(name))
    SIGNAL_TABLE
#undef SIGNAL_TABLE_ENTRY

    // Perf Counters
#ifdef CONFIG_PERF_COUNTERS
#define PERF_COUNTER_TABLE_ENTRY(name) BIND_SIGNAL(perf_counters.name, TOSTRING(name))
    PERF_COUNTER_TABLE
#undef PERF_COUNTER_TABLE_ENTRY
#else
    static uint64_t invalid_binding = 0;
#define PERF_COUNTER_TABLE_ENTRY(name) bindings.perf_counters.name = &invalid_binding;
    PERF_COUNTER_TABLE
#undef PERF_COUNTER_TABLE_ENTRY
#endif

#undef BIND_SIGNAL
}

uint64_t CPUProxy::inst_count() const
{
    return *bindings.perf_counters.all_ops;
}

uint64_t CPUProxy::cycle_count() const
{
    return *bindings.perf_counters.all_cycles;
}

uint32_t CPUProxy::reg(uint32_t idx) const
{
    return *bindings.gprs[idx];
}

uint32_t CPUProxy::csr(uint32_t idx) const
{
    if (bindings.csrs[idx] == nullptr)
    {
        printf("Reading unknown CSR: 0x%x\n", idx);
        return 0;
    }
    return *bindings.csrs[idx];
}

bool CPUProxy::is_csr_valid(uint32_t idx) const
{
    return bindings.csrs[idx] != nullptr;
}

void CPUProxy::dump_gprs(FILE* stream) const
{
    for (int i = 0; i < 16; i++)
        fprintf(stream, "x%-2d %-5s  0x%08x  %11d\n", i, gpr_names[i], reg(i), reg(i));
}

void CPUProxy::dump_csrs(FILE* stream) const
{
    for (int i = 0; i < 4096; i++)
    {
        if (csr_names[i] == nullptr)
            continue;
        fprintf(stream, "%-10s 0x%08x  %11d\n", csr_names[i], csr(i), csr(i));
    }
}

void CPUProxy::dump_perf_counters(FILE* stream)
{
#ifdef CONFIG_PERF_COUNTERS
    auto& b = bindings.perf_counters;
#define PERF(name) fprintf(stream, TOSTRING(name) " = %lu\n", *b.name)
    PERF(ifu_fetched);
    PERF(lsu_read);
    PERF(lsu_write);
    PERF(all_ops);
    PERF(all_cycles);
    PERF(icache_hit);
    PERF(icache_miss);
#undef PERF

    auto all_ops_d = static_cast<double>(*b.all_ops);
    auto all_cycles_d = static_cast<double>(*b.all_cycles);

    fprintf(stream, "+----------+----------+--------+\n");
    fprintf(stream, "| Type     |    Count | %%      |\n");
    fprintf(stream, "+----------+----------+--------+\n");
#define PERF(display_name, name)  fprintf(stream, "| %-8s | %8lu | %05.2f%% |\n", \
    TOSTRING(display_name), \
    *b.name##_ops, \
    100.0 * (static_cast<double>(*b.name##_ops) / all_ops_d))

    PERF(ALU, alu);
    PERF(Branch, br);
    PERF(LSU, lsu);
    PERF(CSR, csr);
    PERF(Other, other);
#undef PERF
    fprintf(stream, "+----------+----------+--------+\n");

    // AMAT = p * access_time + (1 - p) * (access_time + miss_penalty) = access_time + (1 - p) * miss_penalty
    auto access_time = 1;
    auto miss_penalty = static_cast<double>(*b.icache_mem_access_cycles) / static_cast<double>(*b.icache_miss);
    auto hit_rate = static_cast<double>(*b.icache_hit) / static_cast<double>(*b.ifu_fetched);
    auto AMAT = hit_rate * access_time + (1.0 - hit_rate) * (access_time + miss_penalty);
    fprintf(stream, "icache hit rate = %f\n", hit_rate);
    fprintf(stream, "icache miss penalty = %f\n", miss_penalty);
    fprintf(stream, "icache AMAT = %f\n", AMAT);

    auto idu_hazard = *b.idu_hazard_stall_cycles;
    auto idu_hazard_percent = (static_cast<double>(idu_hazard) / all_cycles_d) * 100.0;
    fprintf(stream, "IDU hazard stalled = %lu (%.2f %%)\n", idu_hazard, idu_hazard_percent);

    auto mispredicted_br = *b.mispredicted_branches;
    auto mispredicted_br_percent = (static_cast<double>(mispredicted_br) / static_cast<double>(*b.br_ops)) * 100.0;
    fprintf(stream, "Mispredicted branches = %lu (%.2f %%)\n", mispredicted_br, mispredicted_br_percent);
#endif
}

void CPUProxy::dump(FILE* stream)
{
    fprintf(stream, "Dumping CPU state:\n");
    fprintf(stream, "exu_pc=0x%08x\n", exu_pc());
    fprintf(stream, "exu_inst=0x%08x\n", exu_inst());
    fprintf(stream, "difftest_pc=0x%08x\n", difftest_pc());
    fprintf(stream, "difftest_inst=0x%08x\n", difftest_inst());
    fprintf(stream, "Registers:\n");
    dump_gprs(stream);
    fprintf(stream, "CSRs:\n");
    dump_csrs(stream);
    fprintf(stream, "Perfeormance Counters:\n");
    dump_perf_counters();
}

void DUTMemory::init(const std::string& filename)
{
    printf("Initializing memory from %s\n", filename.c_str());
    FILE* fp = fopen(filename.c_str(), "rb");
    assert(fp);

    mrom_data = static_cast<uint8_t*>(malloc(CONFIG_MROM_SIZE));
    memset(mrom_data, 0, CONFIG_MROM_SIZE);

    flash_data = static_cast<uint8_t*>(malloc(CONFIG_FLASH_SIZE));
    memset(flash_data, 0, CONFIG_FLASH_SIZE);

    psram_data = static_cast<uint8_t*>(malloc(CONFIG_PSRAM_SIZE));
    memset(psram_data, 0, CONFIG_PSRAM_SIZE);

    sdram_data = static_cast<uint8_t*>(malloc(CONFIG_SDRAM_SIZE));
    memset(sdram_data, 0, CONFIG_SDRAM_SIZE);

    // ATTENTION: guest_to_host must be used after the initialization of `***_data`.
    auto dest_ptr = guest_to_host(RESET_VECTOR);
    auto [beg, end] = get_memory_area(RESET_VECTOR);
    auto dest_size = end - beg;

    size_t bytes_read = fread(dest_ptr, 1, dest_size, fp);
    if (bytes_read == 0)
    {
        if (ferror(stdin))
        {
            perror("fread");
            exit(-1);
        }
    }

    inst_memory_size = bytes_read;

    printf("Read %zu bytes from %s\n", bytes_read, filename.c_str());

    // printf("First 32 bytes:\n");
    // for (int i = 0; i < 4; i++)
    //     printf("%08x: %08x\n", i * 4, data[i]);

    fclose(fp);
}

void DUTMemory::destroy()
{
    if (flash_data)
    {
        free(flash_data);
        flash_data = nullptr;
    }

    if (mrom_data)
    {
        free(mrom_data);
        mrom_data = nullptr;
    }

    if (psram_data)
    {
        free(psram_data);
        psram_data = nullptr;
    }

    if (sdram_data)
    {
        free(sdram_data);
        sdram_data = nullptr;
    }
}

void DUTMemory::out_of_bound_abort(uint32_t addr)
{
    auto& cpu = SIM.cpu();
    printf("Out of bound memory access at addr=0x%08x\n, ifu_pc=0x%08x, lsu_pc=0x%08x",
           addr, cpu.ifu_pc(), cpu.lsu_pc());
    cpu.dump();
    SIM.cleanup();
    exit(-1);
}


bool DUTMemory::in_mrom(uint32_t addr)
{
    return addr - CONFIG_MROM_BASE < CONFIG_MROM_SIZE;
}

bool DUTMemory::in_sram(uint32_t addr)
{
    return addr - CONFIG_SRAM_BASE < CONFIG_SRAM_SIZE;
}

bool DUTMemory::in_flash(uint32_t addr)
{
    return addr - CONFIG_FLASH_BASE < CONFIG_FLASH_SIZE;
}

bool DUTMemory::in_psram(uint32_t addr)
{
    return addr - CONFIG_PSRAM_BASE < CONFIG_PSRAM_SIZE;
}

bool DUTMemory::in_sdram(uint32_t addr)
{
    return addr - CONFIG_SDRAM_BASE < CONFIG_SDRAM_SIZE;
}

bool DUTMemory::in_device(uint32_t addr)
{
    return !in_mrom(addr) && !in_sram(addr) && !in_flash(addr) && !in_psram(addr) && !in_sdram(addr);
}

bool DUTMemory::in_sim_mem(uint32_t addr)
{
    return in_flash(addr) || in_mrom(addr) || in_psram(addr) || in_sdram(addr);
}

std::tuple<uint32_t, uint32_t> DUTMemory::get_memory_area(uint32_t addr)
{
    if (in_mrom(addr))
        return {CONFIG_MROM_BASE, CONFIG_MROM_BASE + CONFIG_MROM_SIZE};
    if (in_flash(addr))
        return {CONFIG_FLASH_BASE, CONFIG_FLASH_BASE + CONFIG_FLASH_SIZE};
    if (in_psram(addr))
        return {CONFIG_PSRAM_BASE, CONFIG_PSRAM_BASE + CONFIG_PSRAM_SIZE};
    if (in_sdram(addr))
        return {CONFIG_SDRAM_BASE, CONFIG_SDRAM_BASE + CONFIG_SDRAM_SIZE};

    assert(0);
    return {};
}

uint8_t* DUTMemory::guest_to_host(uint32_t paddr) const
{
    if (in_mrom(paddr))
        return reinterpret_cast<uint8_t*>(mrom_data) + paddr - CONFIG_MROM_BASE;
    if (in_flash(paddr))
        return reinterpret_cast<uint8_t*>(flash_data) + paddr - CONFIG_FLASH_BASE;
    if (in_psram(paddr))
        return reinterpret_cast<uint8_t*>(psram_data) + paddr - CONFIG_PSRAM_BASE;
    if (in_sdram(paddr))
        return reinterpret_cast<uint8_t*>(sdram_data) + paddr - CONFIG_SDRAM_BASE;

    assert(0 && "Unknown memory region");
    return nullptr;
}

uint32_t DUTMemory::host_to_guest(uint8_t* haddr) const
{
    auto haddr_u32 = in_flash(static_cast<uint32_t>(reinterpret_cast<uint64_t>(haddr)));
    if (in_mrom(haddr_u32))
        return haddr - reinterpret_cast<uint8_t*>(mrom_data) + CONFIG_MROM_BASE;
    if (in_flash(haddr_u32))
        return haddr - reinterpret_cast<uint8_t*>(flash_data) + CONFIG_FLASH_BASE;
    if (in_psram(haddr_u32))
        return haddr - reinterpret_cast<uint8_t*>(psram_data) + CONFIG_PSRAM_BASE;
    if (in_sdram(haddr_u32))
        return haddr - reinterpret_cast<uint8_t*>(sdram_data) + CONFIG_SDRAM_BASE;

    assert(0 && "Unknown memory region");
    return 0;
}

void SimHandle::init_trace()
{
#ifdef TRACE
    tfp = new TFP_TYPE;
    Verilated::traceEverOn(true);
    dut->trace(tfp, 0);
    tfp->open(TOSTRING(TRACE_FILENAME));
#endif
}

void SimHandle::cleanup_trace()
{
#ifdef TRACE
    if (tfp)
    {
        tfp->close();
        delete tfp;
        tfp = nullptr;
    }
#endif
}

void SimHandle::init_sim(TOP_NAME* dut_, const char* img_path_, const char* statistics_path_)
{
    // img_path can not be null
    assert(img_path_ != nullptr);
    img_path = img_path_;
    // statistic_path is optional
    statistics_path = statistics_path_ ? statistics_path_ : "";

    dut = dut_;
    cpu_proxy.bind(dut_);
    cycle_counter = 0;
    sim_time = 0;

    init_trace();

    memory.init(img_path);

    boot_timepoint = std::chrono::high_resolution_clock::now();

#define CSR_TABLE_ENTRY(name, idx) csr_names[idx] = #name;
    CSR_TABLE
#undef CSR_TABLE_ENTRY
}

void SimHandle::cleanup()
{
    cleanup_trace();
    memory.destroy();
}

void SimHandle::single_cycle()
{
    assert(!has_got_ebreak() && "single_cycle called after ebreak.");

    dut->clock = 1;
    dut->eval();

    IFDEF(TRACE, tfp->dump(sim_time++));

    dut->clock = 0;
    dut->eval();

    IFDEF(TRACE, tfp->dump(sim_time++));

    cycle_counter++;
}

void SimHandle::drain()
{
    while (!got_ebreak)
        single_cycle();
}


void SimHandle::reset(int n)
{
    dut->clock = 0;
    dut->reset = 1;
    while (n-- > 0)
    {
        dut->clock = 1;
        dut->eval();

        dut->clock = 0;
        dut->eval();
    }
    dut->reset = 0;
}

void SimHandle::dump_after_ebreak()
{
    auto a0 = cpu().reg(10);
    auto exu_pc = cpu().exu_pc();
    if (a0 == 0)
        printf("\33[1;32mHIT GOOD TRAP\33[0m at exu_pc = 0x%x\n", exu_pc);
    else
        printf("\33[1;41mHIT BAD TRAP\33[0m at exu_pc = 0x%x, a0=%d\n", exu_pc, a0);

    printf("Ebreak after %lu cycles\n", simulator_cycles());
    printf("Statistics:\n");
    dump_statistics(stdout);
    dump_statistics_json();
}

void SimHandle::dump_statistics(FILE* stream) const
{
    auto cnt_d = static_cast<double>(cpu().inst_count());
    auto cycles_d = static_cast<double>(cpu().cycle_count());

    fprintf(stream, "Elapsed time: %lu us\n", elapsed_time());

    double cycle_per_us = static_cast<double>(elapsed_time()) / cycles_d;
    fprintf(stream, "Cycles per us: %f\n", cycle_per_us);

    fprintf(stream, "Cycles Per Inst (CPI) = %f\n", cycles_d / cnt_d);
    fprintf(stream, "Insts Per Cycle (IPC) = %f\n", cnt_d / cycles_d);

    fprintf(stream, "Perf Counters:\n");
    SIM.cpu().dump_perf_counters(stream);
}

void SimHandle::dump_statistics_json(FILE* stream) const
{
    if (stream == nullptr)
    {
        stream = fopen(statistics_path.c_str(), "w");
        if (stream == nullptr)
        {
            fprintf(stderr, "Warning: Can not open statistic file: '%s'. Printing to stderr...\n",
                    statistics_path.c_str());
            stream = stderr;
        }
        else
            fprintf(stdout, "Dumping statistics to file: '%s'\n", statistics_path.c_str());
    }

    using KeyT = std::string;
    using ValT = std::variant<std::string, uint64_t, double>;

    std::vector<std::pair<KeyT, ValT>> data;
    auto emit = [&](const std::string& name, const ValT& val)
    {
        data.emplace_back(name, val);
    };

    auto& c = SIM.cpu();

    emit("mode", mode);
    emit("trace", trace_mode);
    emit("image_path", img_path);
    emit("elapsed_time", elapsed_time());
    emit("simulator_cycles", simulator_cycles());

#define PERF_COUNTER_TABLE_ENTRY(name) emit(TOSTRING(name), *c.bindings.perf_counters.name);
    PERF_COUNTER_TABLE
#undef PERF_COUNTER_TABLE_ENTRY

    fprintf(stream, "{\n");
    for (auto it = data.begin(); it != data.end(); ++it)
    {
        const auto& [key, val] = *it;

        // Ident
        fprintf(stream, "  ");

        // Key
        fprintf(stream, "\"%s\": ", key.c_str());

        // Val
        if (auto str = std::get_if<std::string>(&val))
            fprintf(stream, "\"%s\"", str->c_str());
        else if (auto u64 = std::get_if<uint64_t>(&val))
            fprintf(stream, "%lu", *u64);
        else if (auto d = std::get_if<double>(&val))
            fprintf(stream, "%f", *d);
        else
            assert(false && "Unknown type.");

        // ,
        if (std::next(it) != data.end())
            fprintf(stream, ",\n");
    }
    fprintf(stream, "\n}\n");
}
