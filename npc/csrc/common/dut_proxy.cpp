#include "config.hpp"
#include "dut_proxy.hpp"
#include <iostream>
#include <iomanip>

TOP_NAME DUT;
SimHandle SIM;
const char* csr_names[4096];
const char* gpr_names[32];
void CPUProxy::bind(TOP_NAME* this_dut)
{
#define CORE(x) &this_dut->rootp->ysyxSoCFull__DOT__asic__DOT__cpu__DOT__cpu__DOT__core__DOT__##x
#define BIND(reg) register_bindings[reg] = CORE(RegFile__DOT__regs_##reg);
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

    pc_binding = CORE(IFU__DOT__pc);
    ifu_state_binding = CORE(IFU__DOT__state);
    dnpc_binding = CORE(_WBU_io_out_bits_dnpc);
    inst_binding = CORE(IDU__DOT__inst);
    difftest_ready_binding = CORE(IFU__DOT__difftest_ready);

    // We can't use CSR_TABLE_ENTRY here because some csr needs special handling
#define BIND(name) csr_bindings[CSR_##name] = CORE(EXU__DOT__csr_file__DOT__##name);

    BIND(mstatus)
    BIND(mtvec)
    BIND(mepc)
    BIND(mcause)

#undef BIND
    auto mcycle = CORE(EXU__DOT__csr_file__DOT__mcycle);
    static uint32_t mvendorid = 0x79737978;
    static uint32_t marchid = 25100251;

    uint32_t* mcycle32 = reinterpret_cast<uint32_t*>(mcycle);
    csr_bindings[CSR_mcycle] = mcycle32;
    csr_bindings[CSR_mcycleh] = mcycle32 + 1;
    csr_bindings[CSR_mvendorid] = &mvendorid;
    csr_bindings[CSR_marchid] = &marchid;

    // Safety checks
#define CSR(x) TOSTRING(x)
    // It will be expanded to
    //     (static_cast<bool>(csr_bindings[0x300] != nullptr && "mstatus" " not initialized.")
    // ? void(0)
    // : __assert_fail("csr_bindings[0x300] != nullptr && CSR(mstatus) \" is not initialized.\"", __builtin_FILE(),
    //                 __builtin_LINE(), __PRETTY_FUNCTION__));
#define CSR_TABLE_ENTRY(name, idx) assert(csr_bindings[idx] != nullptr && CSR(name) " is not initialized.");
    CSR_TABLE
#undef CSR_TABLE_ENTRY
#undef CSR

#undef CORE
}

uint32_t CPUProxy::curr_inst() const
{
    return *inst_binding;
}

uint32_t CPUProxy::pc() const
{
    return *pc_binding;
}

uint32_t CPUProxy::dnpc() const
{
    return *dnpc_binding;
}

uint32_t CPUProxy::reg(uint32_t idx) const
{
    return *register_bindings[idx];
}

uint32_t CPUProxy::csr(uint32_t idx) const
{
    if (csr_bindings[idx] == nullptr)
    {
        printf("Reading unknown CSR: 0x%x\n", idx);
        return 0;
    }
    return *csr_bindings[idx];
}

bool CPUProxy::is_csr_valid(uint32_t idx) const
{
    return csr_bindings[idx] != nullptr;
}

bool CPUProxy::is_ready_for_difftest() const
{
    return *difftest_ready_binding;
}

bool CPUProxy::is_inst_valid() const
{
    return *ifu_state_binding == 2;
}

void CPUProxy::dump_gprs(std::ostream& os)
{
    for (int i = 0; i < 16; i++)
        printf("x%-2d %-5s  0x%08x  %11d\n", i, gpr_names[i], reg(i), reg(i));
}

void CPUProxy::dump_csrs(std::ostream& os)
{
    for (int i = 0; i < 4096; i++)
    {
        if (csr_names[i] == nullptr)
            continue;
        printf("%-10s 0x%08x  %11d\n", csr_names[i], csr(i), csr(i));
    }
}

void CPUProxy::dump(std::ostream& os)
{
    os << "Registers:\n";
    dump_gprs(os);
    os << "CSRs:\n";
    dump_csrs(os);
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
    cpu.dump(std::cerr);
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
    DUT.trace(tfp, 0);
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

void SimHandle::init_sim(const std::string& filename)
{
    cpu_proxy.bind(&DUT);
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
    DUT.clock = 1;
    DUT.eval();

    IFDEF(TRACE, tfp->dump(sim_time++));

    DUT.clock = 0;
    DUT.eval();

    IFDEF(TRACE, tfp->dump(sim_time++));

    cycle_counter++;
}

void SimHandle::reset(int n)
{
    DUT.clock = 0;
    DUT.reset = 1;
    while (n-- > 0)
    {
        DUT.clock = 1;
        DUT.eval();

        DUT.clock = 0;
        DUT.eval();
    }
    DUT.reset = 0;
}
