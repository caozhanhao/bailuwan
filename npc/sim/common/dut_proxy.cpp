// Copyright (c) 2025 caozhanhao
// SPDX-License-Identifier: MIT

#include "config.hpp"
#include "dut_proxy.hpp"

#include <verilated.h>
#include <verilated_syms.h>

#include <iostream>
#include <iomanip>
#include <type_traits>

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

    // Normal Signals
    BIND_SIGNAL(pc, "pc")
    BIND_SIGNAL(ifu_state, "ifu_state")
    BIND_SIGNAL(dnpc, "dnpc")
    BIND_SIGNAL(inst, "inst")
    BIND_SIGNAL(difftest_ready, "difftest_ready")

    // Perf Counters
#define PERF_COUNTER_TABLE_ENTRY(name) BIND_SIGNAL(name, STRINGIFY(name))
    PERF_COUNTER_TABLE
#undef PERF_COUNTER_TABLE_ENTRY

    // CSRs
#define CSR_TABLE_ENTRY(name, idx) BIND_SIGNAL(csrs[idx], STRINGIFY(name))
    CSR_TABLE
#undef CSR_TABLE_ENTRY

#undef BIND_SIGNAL
}

uint32_t CPUProxy::curr_inst() const
{
    return *bindings.inst;
}

uint64_t CPUProxy::inst_count() const
{
    return *bindings.all_ops;
}

uint32_t CPUProxy::pc() const
{
    return *bindings.pc;
}

uint32_t CPUProxy::dnpc() const
{
    return *bindings.dnpc;
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

bool CPUProxy::is_ready_for_difftest() const
{
    return *bindings.difftest_ready;
}

bool CPUProxy::is_inst_valid() const
{
    return *bindings.ifu_state == 2;
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
    auto& b = bindings;
#define PERF(name) fprintf(stream, STRINGIFY(name) " = %lu\n", *b.name)
    PERF(ifu_fetched);
    PERF(lsu_read);
    PERF(exu_done);
    PERF(all_ops);
    PERF(all_cycles);
    PERF(wait_cycles);
#undef PERF

    auto all_ops_d = static_cast<double>(*b.all_ops);

    fprintf(stream, "+----------+----------+--------+------------+\n");
    fprintf(stream, "| Type     |    Count | %%      | Avg Cycles |\n");
    fprintf(stream, "+----------+----------+--------+------------+\n");
#define PERF(display_name, name)  fprintf(stream, "| %-8s | %8lu | %05.2f%% | %10.2f |\n", \
    STRINGIFY(display_name), \
    *b.name##_ops, \
    100.0 * (static_cast<double>(*b.name##_ops) / all_ops_d), \
    (static_cast<double>(*b.name##_cycles) / static_cast<double>(*b.name##_ops)))

    PERF(ALU, alu);
    PERF(Branch, br);
    PERF(LSU, lsu);
    PERF(CSR, csr);
    PERF(Other, other);
#undef PERF
    fprintf(stream, "+----------+----------+--------+------------+\n");
}

void CPUProxy::dump(FILE* stream)
{
    fprintf(stream, "Dumping CPU state:\n");
    fprintf(stream, "PC=0x%08x\n", pc());
    fprintf(stream, "Inst=0x%08x\n", curr_inst());
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

    size_t bytes_read = fread(flash_data, 1, CONFIG_FLASH_SIZE, fp);
    if (bytes_read == 0)
    {
        if (ferror(stdin))
        {
            perror("fread");
            free(flash_data);
            exit(-1);
        }
    }

    inst_memory_size = CONFIG_FLASH_SIZE;

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
    printf("Out of bound memory access at PC = 0x%08x, addr = 0x%08x\n", cpu.pc(), addr);
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

void SimHandle::init_sim(TOP_NAME* dut_, const std::string& filename)
{
    dut = dut_;
    cpu_proxy.bind(dut_);
    cycle_counter = 0;
    sim_time = 0;
    init_trace();

    memory.init(filename);

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
    dut->clock = 1;
    dut->eval();

    IFDEF(TRACE, tfp->dump(sim_time++));

    dut->clock = 0;
    dut->eval();

    IFDEF(TRACE, tfp->dump(sim_time++));

    cycle_counter++;
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

void SimHandle::dump_statistics(FILE* stream)
{
    auto inst_count = cpu().inst_count();
    auto cnt_d = static_cast<double>(inst_count);
    auto cycles_d = static_cast<double>(cycles());

    fprintf(stream, "Elapsed time: %lu us\n", elapsed_time());

    double cycle_per_us = static_cast<double>(elapsed_time()) / cycles_d;
    fprintf(stream, "Cycles per us: %f\n", cycle_per_us);

    fprintf(stream, "Cycles Per Inst (CPI) = %f\n", cycles_d / cnt_d);
    fprintf(stream, "Insts Per Cycle (IPC) = %f\n", cnt_d / cycles_d);

    fprintf(stream, "Perf Counters:\n");
    SIM.cpu().dump_perf_counters(stream);
}
