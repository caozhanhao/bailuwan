#include "config.hpp"
#include "dut_proxy.hpp"
#include <iostream>
#include <iomanip>

TOP_NAME dut;
SimHandle sim_handle;
const char *csr_names[4096];

void CPUProxy::bind(TOP_NAME* this_dut)
{
#define BIND(reg) register_bindings[reg] = &this_dut->io_registers_##reg;
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

    pc_binding = &this_dut->io_pc;
    dnpc_binding = &this_dut->io_dnpc;
    inst_binding = &this_dut->io_inst;

#define CSR_TABLE_ENTRY(name, idx) csr_bindings[idx] = &this_dut->io_csrs_##name;
    CSR_TABLE
#undef CSR_TABLE_ENTRY
}

uint32_t CPUProxy::curr_inst()
{
    return *inst_binding;
}

uint32_t CPUProxy::pc()
{
    return *pc_binding;
}

uint32_t CPUProxy::dnpc()
{
    return *dnpc_binding;
}

uint32_t CPUProxy::reg(uint32_t idx)
{
    return *register_bindings[idx];
}

uint32_t CPUProxy::csr(uint32_t idx)
{
    if (csr_bindings[idx] == nullptr)
    {
        printf("Reading unknown CSR: 0x%x\n", idx);
        return 0;
    }
    return *csr_bindings[idx];
}

bool CPUProxy::is_csr_valid(uint32_t idx)
{
    return csr_bindings[idx] != nullptr;
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

    data = static_cast<uint32_t*>(malloc(CONFIG_MSIZE));
    memset(data, 0, CONFIG_MSIZE);

    size_t bytes_read = fread(data, 1, CONFIG_MSIZE, fp);
    if (bytes_read == 0)
    {
        if (ferror(stdin))
        {
            perror("fread");
            free(data);
            exit(-1);
        }
    }

    img_size = CONFIG_MSIZE;

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
    if (uaddr == RTC_MMIO || uaddr == RTC_MMIO + 4)
    {
        auto now = std::chrono::high_resolution_clock::now();
        auto delta = now - sim_handle.get_boot_time();
        auto sec = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(delta).count());
        if (uaddr == RTC_MMIO)
            return sec & 0xffffffff;
        return sec >> 32;
    }
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
    if (!in_pmem(uaddr))
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

    // Serial port
    if (uaddr == SERIAL_PORT_MMIO && wmask == 1)
    {
        putchar(wdata);
        fflush(stdout);
        return;
    }

    // Memory
    if (!in_pmem(uaddr))
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

bool DUTMemory::in_pmem(uint32_t addr)
{
    return addr - CONFIG_MBASE < CONFIG_MSIZE;
}

uint8_t* DUTMemory::guest_to_host(uint32_t paddr) const
{
    return reinterpret_cast<uint8_t*>(data) + paddr - CONFIG_MBASE;
}

uint32_t DUTMemory::host_to_guest(uint8_t* haddr) const
{
    return haddr - reinterpret_cast<uint8_t*>(data) + CONFIG_MBASE;
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
    IFDEF(TRACE, tfp->dump(sim_time++));

    dut.clock = 0;
    dut.eval();

    IFDEF(TRACE, tfp->dump(sim_time++));

    dut.clock = 1;
    dut.eval();

    cycle_counter++;
}

void SimHandle::reset(int n)
{
    dut.reset = 1;
    while (n-- > 0)
        single_cycle();
    dut.reset = 0;
}
