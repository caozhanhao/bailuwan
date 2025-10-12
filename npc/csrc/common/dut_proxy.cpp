#include "dut_proxy.hpp"
#include <iostream>
#include <iomanip>

TOP_NAME dut;
SimHandle sim_handle;

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
    inst_binding = &this_dut->io_inst;
}

uint32_t CPUProxy::curr_inst()
{
    return *inst_binding;
}

uint32_t CPUProxy::pc()
{
    return *pc_binding;
}

uint32_t CPUProxy::reg(uint32_t idx)
{
    return *register_bindings[idx];
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

void DUTMemory::init(const std::string& filename, uint32_t addr_base_)
{
    addr_base = addr_base_;

    printf("Initializing memory from %s\n", filename.c_str());
    FILE* fp = fopen(filename.c_str(), "rb");
    assert(fp);

    data = static_cast<uint32_t*>(malloc(DUT_MEMORY_MAXSIZE));
    memset(data, 0, DUT_MEMORY_MAXSIZE);

    size_t bytes_read = fread(data, 1, DUT_MEMORY_MAXSIZE, fp);
    if (bytes_read == 0)
    {
        if (ferror(stdin))
        {
            perror("fread");
            free(data);
            exit(-1);
        }
    }

    size = DUT_MEMORY_MAXSIZE;

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
#define RTC_MMIO 0xa0000048
#define SERIAL_PORT_MMIO 0x10000000
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
    auto& mem = sim_handle.get_memory();
    auto& cpu = sim_handle.get_cpu();

    uaddr -= sim_handle.get_memory().addr_base;
    uint32_t idx = uaddr / 4u;

    if (idx >= mem.size)
    {
        printf("Out of bound memory access at PC = 0x%08x, raddr = 0x%08x\n", cpu.pc(), raddr);
        cpu.dump_registers(std::cerr);
        exit(-1);
    }

    return mem.data[idx];
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
    auto& mem = sim_handle.get_memory();
    auto& cpu = sim_handle.get_cpu();

    uaddr -= sim_handle.get_memory().addr_base;
    uint32_t idx = uaddr / 4;

    if (idx >= mem.size)
    {
        printf("Out of bound memory access at PC = 0x%08x, waddr = 0x%08x\n", cpu.pc(), waddr);
        cpu.dump_registers(std::cerr);
        exit(-1);
    }

    uint32_t cur = mem.data[idx];
    uint32_t newv = cur;
    uint32_t wd = static_cast<uint32_t>(wdata);
    uint8_t mask = static_cast<uint8_t>(wmask);

    for (int i = 0; i < 4; ++i)
    {
        if (mask & (1u << i))
        {
            uint32_t byte_mask = 0xFFu << (i * 8);
            uint32_t src_byte = (wd >> (i * 8)) & 0xFFu;
            newv = (newv & ~byte_mask) | (src_byte << (i * 8));
        }
    }

    mem.data[idx] = newv;
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
    tfp->close();
    delete tfp;
#endif
}

void SimHandle::init_sim(const std::string& filename, uint32_t addr_base_)
{
    cpu.bind(&dut);
    cycle_counter = 0;
    sim_time = 0;
    init_trace();

    memory.init(filename.c_str(), addr_base_);

    boot_time = std::chrono::high_resolution_clock::now();
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
