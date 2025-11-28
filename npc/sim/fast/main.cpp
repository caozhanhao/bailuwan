#include "dut_proxy.hpp"
#include "utils/disasm.hpp"

#include <getopt.h>

static const char* img_file = nullptr;

// Compatible with SDB
static void parse_args(int argc, char* argv[])
{
    const struct option table[] = {
        {"batch", no_argument, nullptr, 'b'},
        {"elf", required_argument, nullptr, 'e'},
        {"help", no_argument, nullptr, 'h'},
        {0, 0, nullptr, 0},
    };
    int o;
    while ((o = getopt_long(argc, argv, "-bhe:", table, nullptr)) != -1)
    {
        switch (o)
        {
        case 'e':
        case 'b':
            printf("Ignoring option -%c\n", o);
            break;
        case 1:
            img_file = optarg;
            return;
        default:
            printf("Usage: %s [filename]\n", argv[0]);
            exit(0);
        }
    }
}

int main(int argc, char* argv[])
{
    Verilated::commandArgs(argc, argv);
    parse_args(argc, argv);
    // INIT
    SIM.init_sim(&DUT, img_file);
    SIM.reset(10);

    // Simulate
    printf("Fast simulation started.\n");

    void bind_by(TOP_NAME* dut);
    bind_by(&DUT);

    try
    {
        while (true)
            SIM.single_cycle();
    }
    catch (EBreakException& e)
    {
        SIM.cleanup();
        exit(e.get_code());
    }

    // never reach here

    return 0;
}
