#ifndef BAILUWAN_TRACESIM_BRANCHSIM_H
#define BAILUWAN_TRACESIM_BRANCHSIM_H

#include <cstdint>
#include <cstdio>
#include <vector>

class BranchSim
{
private:
    struct BTBEntry
    {
        bool valid;
        uint64_t tag;
        uint64_t target;
        bool is_uncond;

        BTBEntry() : valid(false), tag(0), target(0), is_uncond(false)
        {
        }
    };

    const int entries;
    const int index_bits;
    const uint64_t index_mask;

    std::vector<BTBEntry> table;

    uint64_t total_branches = 0;
    uint64_t mispredictions = 0;


    static uint32_t log2_u32(uint32_t n)
    {
        uint32_t r = 0;
        while ((n >>= 1) != 0) r++;
        return r;
    }

public:
    BranchSim(const uint8_t* image, size_t size, int btb_entries = 32)
        : entries(btb_entries),
          index_bits(log2_u32(btb_entries)),
          index_mask(btb_entries - 1),
          table(btb_entries)
    {
    }

    // 辅助函数：从镜像中获取指令以判断类型
    uint32_t fetch_inst(uint32_t pc)
    {
        // 计算在 image buffer 中的偏移
        if (pc < base_addr) return 0; // 越界保护
        uint64_t offset = pc - base_addr;
        if (offset + 3 >= image_size) return 0;

        // 小端序读取
        const uint8_t* p = image_ptr + offset;
        return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
    }

    // 判断是否为无条件跳转 (JAL, JALR)
    // EXU logic: io.btb_w.is_uncond := decoded.br_op === BrOp.JAL || decoded.br_op === BrOp.JALR
    bool is_unconditional_inst(uint32_t inst)
    {
        uint32_t opcode = inst & 0x7F;
        // JAL = 1101111 (0x6F), JALR = 1100111 (0x67)
        return (opcode == 0x6F) || (opcode == 0x67);
    }

    // 模拟核心逻辑
    void process(uint32_t pc, uint32_t actual_target, bool actual_taken)
    {
        total_branches++;

        // 1. 获取指令信息 (对应 EXU 解码阶段，但在仿真中我们这里需要提前知道以用于更新 logic)
        uint32_t inst = fetch_inst(pc);
        bool is_uncond_inst = is_unconditional_inst(inst);

        // ================= PREDICT STAGE (IFU + BPU + BTB Read) =================
        // Chisel: val r_idx = io.r.pc(INDEX_BITS + 1, 2)
        uint32_t r_idx = (pc >> 2) & index_mask;
        // Chisel: val r_tag = io.r.pc(p.XLEN - 1, INDEX_BITS + 2)
        uint64_t r_tag = pc >> (index_bits + 2);

        // BTB Lookup
        const auto& entry = table[r_idx];
        bool btb_hit = entry.valid && (entry.tag == r_tag);

        // BPU Logic
        // Chisel: io.predict_taken := io.btb_valid && (io.btb_is_uncond || io.btb_target < io.pc)
        bool predict_taken = false;
        uint32_t predict_target = 0;

        if (btb_hit)
        {
            bool backward = entry.target < pc;
            // 注意：如果 target > pc 且不是 uncond，BPU 预测不跳转 (BTFN 策略的一部分)
            if (entry.is_uncond || backward)
            {
                predict_taken = true;
            }
            predict_target = (uint32_t)entry.target;
        }
        // else predict_taken = false (Forward Not Taken implied by miss)

        // ================= CHECK MISPREDICTION =================
        // EXU Logic:
        // val predict_mismatch = (decoded.predict_taken =/= br_taken) ||
        //   (decoded.predict_taken && (decoded.predict_target =/= br_target))

        bool mispredicted = false;
        if (predict_taken != actual_taken)
        {
            mispredicted = true;
        }
        else if (predict_taken && (predict_target != actual_target))
        {
            mispredicted = true;
        }

        if (mispredicted)
        {
            mispredictions++;
        }

        // ================= UPDATE STAGE (EXU + BTB Write) =================
        // Chisel: io.btb_w.en := valid_br
        // 任何传递进来的 branch trace 都是 valid_br

        uint32_t w_idx = (pc >> 2) & index_mask;
        uint64_t w_tag = pc >> (index_bits + 2);

        // Chisel Write Logic (Mux(en, ...))
        table[w_idx].valid = true;
        table[w_idx].tag = w_tag;
        table[w_idx].target = actual_target;
        table[w_idx].is_uncond = is_uncond_inst;
    }

    void dump()
    {
        printf("BranchSim Results:\n");
        printf("  BTB Entries: %d\n", entries);
        printf("  Total branches: %lu\n", total_branches);
        printf("  Mispredictions: %lu\n", mispredictions);
        if (total_branches > 0)
        {
            printf("  Misprediction rate: %.2f%%\n",
                   static_cast<double>(mispredictions) / static_cast<double>(total_branches) * 100.0);
        }
        else
        {
            printf("  Misprediction rate: 0.00%%\n");
        }
    }
};


#endif
