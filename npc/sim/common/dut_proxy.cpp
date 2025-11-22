#include "config.hpp"
#include "dut_proxy.hpp"
#include <iostream>
#include <iomanip>

TOP_NAME DUT;
SimHandle SIM;
const char* csr_names[4096];
const char* gpr_names[32] = {
    "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2", "s0", "s1", "a0",
    "a1", "a2", "a3", "a4", "a5", "a6", "a7", "s2", "s3", "s4", "s5",
    "s6", "s7", "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
};

void CPUProxy::bind(TOP_NAME* this_dut)
{
    auto& b = bindings;
#define CORE(x) &this_dut->rootp->ysyxSoCFull__DOT__asic__DOT__cpu__DOT__cpu__DOT__core__DOT__##x

    // GPRs
#define BIND(reg) b.gprs[reg] = CORE(RegFile__DOT__regs_##reg);
    BIND(0)
    BIND(1)
    BIND(2)
    BIND(3)
    BIND(4)
    BIND(5)
    BIND(6)
    BIND(7)
    BIND(8)
    BIND(9)
    BIND(10)
    BIND(11)
    BIND(12)
    BIND(13)
    BIND(14)
    BIND(15)
#undef BIND

    b.pc = CORE(IFU__DOT__pc);
    b.ifu_state = CORE(IFU__DOT__state);
    b.dnpc = CORE(_WBU_io_out_bits_dnpc);
    b.inst = CORE(IDU__DOT__inst);
    b.difftest_ready = CORE(IFU__DOT__c__DOT__difftest_ready);

    // CSRS

    // We can't use CSR_TABLE_ENTRY here because some csr needs special handling
#define BIND(name) b.csrs[CSR_##name] = CORE(EXU__DOT__csr_file__DOT__##name);

    BIND(mstatus)
    BIND(mtvec)
    BIND(mepc)
    BIND(mcause)

#undef BIND
    auto mcycle = CORE(EXU__DOT__csr_file__DOT__mcycle);
    static uint32_t mvendorid = 0x79737978;
    static uint32_t marchid = 25100251;

    uint32_t* mcycle32 = reinterpret_cast<uint32_t*>(mcycle);
    b.csrs[CSR_mcycle] = mcycle32;
    b.csrs[CSR_mcycleh] = mcycle32 + 1;
    b.csrs[CSR_mvendorid] = &mvendorid;
    b.csrs[CSR_marchid] = &marchid;

    // Safety checks
#define CSR(x) TOSTRING(x)
    // It will be expanded to
    //     (static_cast<bool>(csr_bindings[0x300] != nullptr && "mstatus" " not initialized.")
    // ? void(0)
    // : __assert_fail("csr_bindings[0x300] != nullptr && CSR(mstatus) \" is not initialized.\"", __builtin_FILE(),
    //                 __builtin_LINE(), __PRETTY_FUNCTION__));
#define CSR_TABLE_ENTRY(name, idx) assert(b.csrs[idx] != nullptr && CSR(name) " is not initialized.");
    CSR_TABLE
#undef CSR_TABLE_ENTRY
#undef CSR

    // Perf counters
    b.ifu_fetched = CORE(IFU__DOT__c_1__DOT__ifu_fetched);
    b.lsu_read = CORE(EXU__DOT__lsu__DOT__c__DOT__lsu_read);
    b.exu_done = CORE(EXU__DOT__c__DOT__exu_done);
    b.alu_op = CORE(IDU__DOT__c__DOT__alu_op);
    b.lsu_op = CORE(IDU__DOT__c_1__DOT__lsu_op);
    b.csr_op = CORE(IDU__DOT__c_2__DOT__csr_op);

#undef CORE
}

uint32_t CPUProxy::curr_inst() const
{
    return *bindings.inst;
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

void CPUProxy::dump_gprs(FILE* stream)
{
    for (int i = 0; i < 16; i++)
        fprintf(stream, "x%-2d %-5s  0x%08x  %11d\n", i, gpr_names[i], reg(i), reg(i));
}

void CPUProxy::dump_csrs(FILE* stream)
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
    PERF(alu_op);
    PERF(lsu_op);
    PERF(csr_op);
#undef PERF
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
    fprintf(stream, "Performance Counters:\n");
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


bool DUTMemory::in_mrom(uint32_t addr) const
{
    return addr - CONFIG_MROM_BASE < CONFIG_MROM_SIZE;
}

bool DUTMemory::in_sram(uint32_t addr) const
{
    return addr - CONFIG_SRAM_BASE < CONFIG_SRAM_SIZE;
}

bool DUTMemory::in_flash(uint32_t addr) const
{
    return addr - CONFIG_FLASH_BASE < CONFIG_FLASH_SIZE;
}

bool DUTMemory::in_psram(uint32_t addr) const
{
    return addr - CONFIG_PSRAM_BASE < CONFIG_PSRAM_SIZE;
}

bool DUTMemory::in_sdram(uint32_t addr) const
{
    return addr - CONFIG_SDRAM_BASE < CONFIG_SDRAM_SIZE;
}

bool DUTMemory::in_device(uint32_t addr) const
{
    return !in_mrom(addr) && !in_sram(addr) && !in_flash(addr) && !in_psram(addr) && !in_sdram(addr);
}

bool DUTMemory::in_sim_mem(uint32_t addr) const
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

    memory.init(filename.c_str());

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
