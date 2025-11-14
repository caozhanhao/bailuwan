#include "config.hpp"
#include "dut_proxy.hpp"
#include <iostream>
#include <iomanip>

TOP_NAME dut;
SimHandle sim_handle;
const char* csr_names[4096];

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

void CPUProxy::dump_registers(std::ostream& os)
{
    for (int i = 0; i < 16; i++)
    {
        os << "x" << i << ": 0x" <<
            std::hex << std::setfill('0') << std::setw(8) << reg(i)
            << std::dec << std::endl;
    }
}

void DUTMemory::init(const std::string& filename)
{
    printf("Initializing memory from %s\n", filename.c_str());
    FILE* fp = fopen(filename.c_str(), "rb");
    assert(fp);

    data = static_cast<uint32_t*>(malloc(CONFIG_MROM_BASE));
    memset(data, 0, CONFIG_MROM_BASE);

    size_t bytes_read = fread(data, 1, CONFIG_MROM_BASE, fp);
    if (bytes_read == 0)
    {
        if (ferror(stdin))
        {
            perror("fread");
            free(data);
            exit(-1);
        }
    }

    img_size = CONFIG_MROM_SIZE;

    printf("Read %zu bytes from %s\n", bytes_read, filename.c_str());

    // printf("First 32 bytes:\n");
    // for (int i = 0; i < 4; i++)
    //     printf("%08x: %08x\n", i * 4, data[i]);

    fclose(fp);
}

void DUTMemory::destroy()
{
    if (data)
        free(data);
    data = nullptr;
}

uint32_t DUTMemory::read(uint32_t raddr)
{
    auto uaddr = static_cast<uint32_t>(raddr);
    uaddr &= ~0x3u;

    // Clock

    if (uaddr - RTC_MMIO >= 8 && uaddr - RTC_MMIO <= 28)
    {
        std::time_t t = std::time(nullptr);
        std::tm* now = std::gmtime(&t);
        switch (uaddr - RTC_MMIO)
        {
        case 8:
            return now->tm_sec;
        case 12:
            return now->tm_min;
        case 16:
            return now->tm_hour;
        case 20:
            return now->tm_mday;
        case 24:
            return now->tm_mon + 1;
        case 28:
            return now->tm_year + 1900;
        default: assert(false);
        }
        assert(false);
    }

    // Memory
    // Only for flash
    if (uaddr >= CONFIG_FLASH_SIZE)
    {
        auto& cpu = sim_handle.get_cpu();
        printf("Out of bound memory access at PC = 0x%08x, raddr = 0x%08x\n", cpu.pc(), raddr);
        cpu.dump_registers(std::cerr);
        sim_handle.cleanup();
        exit(-1);
    }

    return *reinterpret_cast<uint32_t*>(guest_to_host(uaddr));
}

void DUTMemory::write(uint32_t waddr, uint32_t wdata, char wmask)
{
    auto uaddr = static_cast<uint32_t>(waddr);
    uaddr &= ~0x3u;

    // Memory
    // Only for flash
    if (waddr >= CONFIG_FLASH_SIZE)
    {
        auto& cpu = sim_handle.get_cpu();
        printf("Out of bound memory access at PC = 0x%08x, waddr = 0x%08x\n", cpu.pc(), waddr);
        cpu.dump_registers(std::cerr);
        sim_handle.cleanup();
        exit(-1);
    }

    auto haddr = guest_to_host(uaddr);
    uint8_t* u8data = reinterpret_cast<uint8_t*>(&wdata);
    for (int i = 0; i < 4; i++)
    {
        if (wmask & (1 << i))
            haddr[i] = u8data[i];
    }
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

bool DUTMemory::in_device(uint32_t addr)
{
    return !in_mrom(addr) && !in_sram(addr) && !in_flash(addr);
}

bool DUTMemory::in_sim_mem(uint32_t addr)
{
    return in_flash(addr);
}


uint8_t* DUTMemory::guest_to_host(uint32_t paddr) const
{
    // MROM, deprecated
    // return reinterpret_cast<uint8_t*>(data) + paddr - CONFIG_MROM_BASE;

    // FLASH:
    // Note that the address we got exactly is 24-bit, since the read request
    // we sent in APBSPI only takes the low 24-bit of the address.
    // Therefore, the high 12-bit is always 0, and we don't need to substract the FLASH_BASE.
    // By the way, difftest in NEMU does not be affected by this, since the read request is emulated
    // by NEMU itself, not a APBSPI in ysyxSoC.
    return reinterpret_cast<uint8_t*>(data) + paddr;
}

uint32_t DUTMemory::host_to_guest(uint8_t* haddr) const
{
    // MROM, deprecated
    // return haddr - reinterpret_cast<uint8_t*>(data) + CONFIG_MROM_BASE;

    // FLASH:
    // See comments in guest_to_host
    return haddr - reinterpret_cast<uint8_t*>(data);
}

void SimHandle::init_trace()
{
#ifdef TRACE
    tfp = new TFP_TYPE;
    Verilated::traceEverOn(true);
    dut.trace(tfp, 0);
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
    cpu.bind(&dut);
    cycle_counter = 0;
    sim_time = 0;
    init_trace();

    memory.init(filename.c_str());

    boot_time = std::chrono::high_resolution_clock::now();

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
    dut.clock = 1;
    dut.eval();

    IFDEF(TRACE, tfp->dump(sim_time++));

    dut.clock = 0;
    dut.eval();

    IFDEF(TRACE, tfp->dump(sim_time++));

    cycle_counter++;
}

void SimHandle::reset(int n)
{
    dut.clock = 0;
    dut.reset = 1;
    while (n-- > 0)
    {
        dut.clock = 1;
        dut.eval();

        dut.clock = 0;
        dut.eval();
    }
    dut.reset = 0;
}
