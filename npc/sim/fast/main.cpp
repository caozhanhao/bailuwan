// Copyright (c) 2025 caozhanhao
// SPDX-License-Identifier: MIT

#include "dut_proxy.hpp"
#include "utils/disasm.hpp"

#include <getopt.h>

static const char* img_file = nullptr;
static const char* statistics_file = nullptr;

// Compatible with SDB
static void parse_args(int argc, char* argv[])
{
    constexpr option table[] = {
        {"batch", no_argument, nullptr, 'b'},
        {"elf", required_argument, nullptr, 'e'},
        {"help", no_argument, nullptr, 'h'},
        {"statistics", no_argument, nullptr, 's'},
        {nullptr, 0, nullptr, 0},
    };
    int o;
    while ((o = getopt_long(argc, argv, "-bhe:s:", table, nullptr)) != -1)
    {
        switch (o)
        {
        case 'e':
        case 'b':
            printf("Ignoring option -%c\n", o);
            break;
        case 's':
            statistics_file = optarg;
            break;
        case 1:
            img_file = optarg;
            return;
        default:
            printf("Usage: %s [filename]\n", argv[0]);
            printf("\t-s,--statistics=STATISTIC_FILE   Save statistics to file.\n");
            exit(0);
        }
    }
}

int main(int argc, char* argv[])
{
    Verilated::commandArgs(argc, argv);
    parse_args(argc, argv);
    // INIT
    SIM.init_sim(&DUT, img_file, statistics_file);
    SIM.reset(10);

    // Simulate
    printf("Fast simulation started.\n");

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
