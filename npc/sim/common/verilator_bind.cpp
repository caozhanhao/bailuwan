#include "VysyxSoCFull.h"
#include "VysyxSoCFull__Syms.h"
#include "verilated.h"
#include "verilated_syms.h"
#include "utils/macro.hpp"

void bind_by(TOP_NAME* dut) {
    auto scope_map = Verilated::scopeNameMap();
    assert(scope_map);
    for (auto& [scope_name, scope] : *scope_map)
    {
        auto varsp = scope->varsp();
        if (!varsp)
            continue;
        for (const auto& [name, var] : *varsp) {
            printf("name: %s\n", name);
        }
    }
}