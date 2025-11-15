#include "dut_proxy.hpp"
#include "utils/disasm.hpp"

#include <getopt.h>

const char* img_file = nullptr;

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
    sim_handle.init_sim(img_file);
    sim_handle.reset(10);

    // Simulate
    printf("Simulation started...\n");
    while (true)
    {
        try
        {
            sim_handle.single_cycle();
        }
        catch (EBreakException& e)
        {
            exit(e.get_code());
        }
    }

    printf("Simulation terminated after %lu cycles\n", sim_handle.get_cycles());

    sim_handle.cleanup();
    return 0;
}
