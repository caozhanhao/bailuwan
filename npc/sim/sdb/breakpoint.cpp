// Copyright (c) 2025 caozhanhao
// SPDX-License-Identifier: MIT

// SDB is adapted from sdb in NEMU.

#include "dut_proxy.hpp"
#include "sdb.hpp"

typedef struct breakpoint
{
    int NO;
    int pool_index;
    struct breakpoint* next;

    word_t addr;
} BP;

static BP bp_pool[NR_BP] = {};
static BP *head = nullptr, *free_ = nullptr;

void init_bp_pool()
{
    for (int i = 0; i < NR_BP; i++)
    {
        // To avoid conflict with watch points, the NO of breakpoints starts from NR_WP.
        bp_pool[i].NO = NR_WP + i;
        bp_pool[i].next = (i == NR_BP - 1 ? nullptr : &bp_pool[i + 1]);
    }

    head = nullptr;
    free_ = bp_pool;
}

BP* new_bp()
{
    Assert(free_, "No more breakpoints available");
    BP* p = free_;
    free_ = free_->next;
    p->next = head;
    head = p;
    return p;
}

void free_bp(BP* bp)
{
    Assert(bp, "Breakpoint is nullptr");

    if (bp == head)
    {
        head = head->next;
        bp->next = free_;
        free_ = bp;
        return;
    }

    for (BP* p = head; p != nullptr; p = p->next)
    {
        if (p->next == bp)
        {
            p->next = bp->next;
            bp->next = free_;
            free_ = bp;
            return;
        }
    }

    panic("Breakpoint not found");
}

void bp_update_one(BP* p)
{
    if (SIM.cpu().exu_pc() == p->addr)
    {
        Log("Breakpoint hit at 0x%x.", p->addr);

        if (sdb_state == SDBState::Running)
            sdb_state = SDBState::Stop;
    }
}

void bp_update()
{
    for (BP* p = head; p != nullptr; p = p->next)
        bp_update_one(p);
}

void bp_display()
{
    BP* buffer[NR_BP] = {};
    int buffer_pos = 0;
    for (BP* p = head; p != nullptr; p = p->next)
        buffer[buffer_pos++] = p;

    printf("%-6s %s\n", "Num", "What");
    for (int i = buffer_pos - 1; i >= 0; --i)
    {
        BP* p = buffer[i];

        uint32_t entry_addr;
        if (auto func = ftrace_search(p->addr, &entry_addr))
            printf("%-6d " FMT_WORD " <@%s+0x%x>\n", p->NO, p->addr, func, p->addr - entry_addr);
        else
            printf("%-6d " FMT_WORD "\n", p->NO, p->addr);
    }
}

void bp_create(word_t addr)
{
    BP* p = new_bp();
    p->addr = addr;

    uint32_t entry_addr;
    if (auto func = ftrace_search(addr, &entry_addr))
        printf("Breakpoint %d: " FMT_WORD " <@%s+0x%x>\n", p->NO, addr, func, addr - entry_addr);
    else
        printf("Breakpoint %d: " FMT_WORD "\n", p->NO, addr);
    bp_update_one(p);
}

void bp_delete(int NO) { free_bp(&bp_pool[NO]); }
