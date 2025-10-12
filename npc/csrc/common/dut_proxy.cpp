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

void DUTMemory::init(const std::string& filename)
{
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


void SimHandle::init_sim(const std::string& filename)
{
    cpu.bind(&dut);
    cycle_counter = 0;
    sim_time = 0;
    init_trace();

    memory.init(filename.c_str());
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
