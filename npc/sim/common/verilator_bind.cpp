#include "VysyxSoCFull.h"
#include "VysyxSoCFull__Syms.h"
#include "verilated.h"
#include "verilated_syms.h"
#include "utils/macro.hpp"

void bind_by(TOP_NAME* dut) {
    auto scope_map = Verilated::scopeNameMap();
    for (auto& [scope_name, scope] : *scope_map)
    {
        for (const auto& [name, var] : *scope->varsp()) {
            printf("name: %s\n", name);
        }
    }
}