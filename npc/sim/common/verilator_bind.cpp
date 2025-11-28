#include "VysyxSoCFull.h"
#include "VysyxSoCFull__Syms.h"
#include "verilated.h"
#include "verilated_syms.h"
#include "utils/macro.hpp"

void bind_by(TOP_NAME* dut) {
    Verilated::scopesDump();
    auto scope = Verilated::scopeFind("ysyxSocFull");

    for (const auto& [name, var] : *scope->varsp()) {
        printf("name: %s\n", name);
    }
}