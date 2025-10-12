#include <iostream>
#include <cstdint>
#include <cstdio>

#include "dut_proxy.hpp"

extern "C" {
void ebreak_handler()
{
    printf("ebreak after %lu cycles\n", sim_handle.get_cycles());
    sim_handle.cleanup();

    auto a0 = sim_handle.get_cpu().reg(10);
    if (a0 == 0)
        printf("\33[1;32mHIT GOOD TRAP\33[0m\n");
    else
        printf("\33[1;41mHIT BAD TRAP\33[0m, a0=%d\n", a0);

    exit(a0);
}

#define RTC_MMIO 0xa0000048
#define SERIAL_PORT_MMIO 0x10000000

int pmem_read(int raddr)
{
    auto uaddr = static_cast<uint32_t>(raddr);
    uaddr &= ~0x3u;

    // Clock
    if (uaddr == RTC_MMIO || uaddr == RTC_MMIO + 4)
    {
        printf("RTC MMIO access at raddr = 0x%08x\n", raddr);
        auto now = std::chrono::high_resolution_clock::now();
        auto delta = now - sim_handle.get_boot_time();
        auto sec = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(delta).count());
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

    uaddr -= 0x80000000u;
    uint32_t idx = uaddr / 4u;

    if (idx >= mem.size)
    {
        printf("Out of bound memory access at PC = 0x%08x, raddr = 0x%08x\n", cpu.pc(), raddr);
        cpu.dump_registers(std::cerr);
        exit(-1);
    }

    return mem.data[idx];
}

void pmem_write(int waddr, int wdata, char wmask)
{
    auto uaddr = static_cast<uint32_t>(waddr);
    uaddr &= ~0x3u;

    // Serial
    if (uaddr == SERIAL_PORT_MMIO && wmask == 1)
    {
        putchar(wdata);
        fflush(stdout);
        return;
    }

    // Memory
    auto& mem = sim_handle.get_memory();
    auto& cpu = sim_handle.get_cpu();

    uaddr -= 0x80000000u;
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
}
