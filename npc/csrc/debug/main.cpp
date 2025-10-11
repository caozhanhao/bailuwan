#include <VTop.h>

#include "trace.hpp"
#include "macro.hpp"
#include "dut_proxy.hpp"
#include "dpic.hpp"

static uint64_t sim_time = 0;

static void single_cycle()
{
    IFDEF(TRACE, tfp->dump(sim_time++));

    dut.clock = 0;
    dut.eval();

    IFDEF(TRACE, tfp->dump(sim_time++));

    dut.clock = 1;
    dut.eval();
}

void reset(int n)
{
    dut.reset = 1;
    while (n-- > 0)
        single_cycle();
    dut.reset = 0;
}

typedef enum
{
    OP_UNKNOWN = 0,
    OP_ADD,
    OP_ADDI,
    OP_LUI,
    OP_LW,
    OP_LBU,
    OP_SW,
    OP_SB,
    OP_JALR,
    OP_EBREAK
} Op;

typedef struct
{
    Op op;

    uint32_t rd;
    uint32_t rs1;
    uint32_t rs2;
    int32_t imm;

    uint32_t encoding;
} Inst;

// MINIRVEMU in E4
void eassert(bool cond, const char* msg)
{
    if (!cond)
    {
        fprintf(stderr, "Assertion failed: %s\n", msg);
        exit(1);
    }
}

int32_t sign_extend_12(uint32_t v)
{
    struct small_int
    {
        int val : 12;
    };

    return (struct small_int){v}.val;
}

Inst decode(uint32_t encoding)
{
    Inst inst;

    inst.encoding = encoding;

    uint32_t opcode = encoding & 0x7f;
    inst.rd = (encoding >> 7) & 0x1f;
    uint32_t funct3 = (encoding >> 12) & 0x7;
    inst.rs1 = (encoding >> 15) & 0x1f;
    inst.rs2 = (encoding >> 20) & 0x1f;
    uint32_t funct7 = (encoding >> 25) & 0x7f;

    switch (opcode)
    {
    case 0b0110011:
        {
            // R-type (ADD)
            eassert(funct3 == 0 && funct7 == 0, "Bad inst");
            inst.op = OP_ADD;
            return inst;
        }

    case 0b0010011:
        {
            // I-type (ADDI)
            eassert(funct3 == 0x0, "Bad inst");
            uint32_t imm12 = (encoding >> 20) & 0xfff;
            int32_t imm = sign_extend_12(imm12);
            inst.imm = imm;
            inst.op = OP_ADDI;
            return inst;
        }
    case 0b0110111:
        {
            // LUI (U-type)
            uint32_t imm20 = encoding & 0xfffff000u;
            inst.imm = (int32_t)(imm20) >> 12;
            inst.op = OP_LUI;
            return inst;
        }
    case 0b0000011:
        {
            // Loads (I-type) LW, LBU ...
            uint32_t imm12 = (encoding >> 20) & 0xfff;
            inst.imm = sign_extend_12(imm12);

            if (funct3 == 0b010)
            {
                // LW
                inst.op = OP_LW;
                return inst;
            }

            if (funct3 == 0b100)
            {
                // LBU
                inst.op = OP_LBU;
                return inst;
            }

            eassert(false, "Bad inst");
            return inst;
        }

    case 0b0100011:
        {
            // Stores (S-type) SW, SB
            uint32_t imm4_5 = (encoding >> 7) & 0x1f;
            uint32_t imm11_7 = (encoding >> 25) & 0x7f;
            uint32_t imm12 = (imm11_7 << 5) | imm4_5;
            inst.imm = sign_extend_12(imm12);

            if (funct3 == 0b010)
            {
                // SW
                inst.op = OP_SW;
                return inst;
            }
            if (funct3 == 0)
            {
                // SB
                inst.op = OP_SB;
                return inst;
            }
            eassert(false, "Bad inst");
            return inst;
        }

    case 0b1100111:
        {
            // JALR (I-type, funct3 == 0)
            eassert(funct3 == 0, "Bad inst");

            uint32_t imm12 = (encoding >> 20) & 0xfff;
            inst.imm = sign_extend_12(imm12);
            inst.op = OP_JALR;
            return inst;
        }

    case 0b1110011:
        {
            inst.op = OP_EBREAK;
            return inst;
        }
    default:
        eassert(false, "Bad inst");
        return inst;
    }
    return inst;
}
void dump(Inst *inst)
{
    char imm_buf[48];
    uint32_t imm_u = (uint32_t)inst->imm;
    snprintf(imm_buf, sizeof(imm_buf), "%d (=0x%x)", inst->imm, imm_u);

    uint32_t regs_to_dump[4];
    int regs_count = 0;

    switch (inst->op)
    {
    case OP_ADD:
        printf("add x%u, x%u, x%u", inst->rd, inst->rs1, inst->rs2);
        regs_to_dump[regs_count++] = inst->rs1;
        regs_to_dump[regs_count++] = inst->rs2;
        break;

    case OP_ADDI:
        printf("addi x%u, x%u, %s", inst->rd, inst->rs1, imm_buf);
        regs_to_dump[regs_count++] = inst->rs1;
        break;

    case OP_LUI:
        printf("lui x%u, %s", inst->rd, imm_buf);
        break;

    case OP_LW:
        printf("lw x%u, %s(x%u)", inst->rd, imm_buf, inst->rs1);
        regs_to_dump[regs_count++] = inst->rs1;
        break;

    case OP_LBU:
        printf("lbu x%u, %s(x%u)", inst->rd, imm_buf, inst->rs1);
        regs_to_dump[regs_count++] = inst->rs1;
        break;

    case OP_SW:
        printf("sw x%u, %s(x%u)", inst->rs2, imm_buf, inst->rs1);
        regs_to_dump[regs_count++] = inst->rs1;
        regs_to_dump[regs_count++] = inst->rs2;
        break;

    case OP_SB:
        printf("sb x%u, %s(x%u)", inst->rs2, imm_buf, inst->rs1);
        regs_to_dump[regs_count++] = inst->rs1;
        regs_to_dump[regs_count++] = inst->rs2;
        break;

    case OP_JALR:
        printf("jalr x%u, %s(x%u)", inst->rd, imm_buf, inst->rs1);
        regs_to_dump[regs_count++] = inst->rs1;
        break;

    case OP_EBREAK:
        printf("ebreak");
        break;

    case OP_UNKNOWN:
    default:
        printf("unknown");
        break;
    }

    printf(" ; enc=0x%08x", inst->encoding);

    for (int i = 0; i < regs_count; ++i) {
        uint32_t r = regs_to_dump[i];
        if (r != 0 && r < 32)
            printf(", x%u=0x%x", r, cpu.reg(r));
    }

    printf(", addr=0x%x\n", cpu.pc() * 4u);
}

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        printf("Usage: %s [number of cycles] [filename]\n", argv[0]);
        return -1;
    }

    int cycles = atoi(argv[1]);

    init_memory(argv[2]);

    trace_init();
    cpu.bind(&dut);

    reset(10);

    printf("Simulation started...\n");
    while (cycles-- > 0)
    {
        printf("0x%08x: 0x%08x\n", cpu.pc(), cpu.curr_inst());
        Inst decoded = decode(cpu.curr_inst());
        dump(&decoded);

        single_cycle();
    }

    trace_cleanup();
    return 0;
}
