#ifndef MINIRVEMU_HPP
#define MINIRVEMU_HPP
#include <cstdint>
#include <iostream>
#include <vector>
#include <string>
#include <ranges>
#include <array>
#include <limits>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <list>
#include <source_location>

void eassert(bool cond, const std::string& msg)
{
    if (!cond)
    {
        std::cerr << msg << "\n";
        throw std::runtime_error("Assertion failed: " + msg);
    }
}

class MiniRVEmu
{
private:
    std::vector<uint32_t> memory;

    uint32_t pc = 0; // word index
    uint32_t regs[32] = {};

public:
    void load(uint32_t* addr, size_t size)
    {
        memory.resize(size);
        memory.assign(addr, addr + size);
    }

    void setPC(uint32_t word_index)
    {
        pc = word_index;
    }

    void step()
    {
        uint32_t inst = fetch();
        auto decoded = decode(inst);
        execute(decoded);
        pc++;
    }

    uint32_t reg(uint32_t r)
    {
        eassert(r < 32, "Invalid register index: " + std::to_string(r));
        return regs[r];
    }

    void run(size_t max_cycles)
    {
        size_t i = 0;

        try
        {
            for (; i < max_cycles; ++i)
            {
                uint32_t inst = fetch();
                auto decoded = decode(inst);
                std::cerr << "Cycle " << i << " Running: " << decoded.dump(this) << "\n";
                execute(decoded);
                pc++;
                eassert(regs[0] == 0, "x0 = " + std::to_string(regs[0]) + "?");
            }
        }
        catch (...)
        {
            std::cerr << "Exception caught at cycle: " << i << "\n";
            dump_regs();
            throw;
        }
    }

    void dump_regs(std::ostream& out = std::cerr) const
    {
        out << "Registers:\n";
        for (int i = 0; i < 32; ++i)
            out << "x" << i << ": 0x" << std::hex << regs[i] << std::dec << "\n";
        out << "pc (word index): " << pc << " (byte addr = " << (pc * 4) <<
            " = 0x" << std::hex << (pc * 4) << std::dec << ")\n";
    }

    void dump_memory(std::ostream& out = std::cerr) const
    {
        out << "Memory:\n";
        for (size_t i = 0; i < memory.size(); ++i)
            out << "0x" << std::hex << i * 4 << ": 0x" << memory[i] << std::dec << "\n";
    }

private:
    uint32_t fetch()
    {
        eassert(pc < memory.size(), "PC out of range.");
        return memory[pc];
    }

    template <size_t bits>
    static int32_t sign_extend(uint32_t v)
    {
        static_assert(bits >= 1 && bits < 32, "extend what?");

        struct small_int
        {
            int64_t val : bits;
        } x = { .val = v };
        return static_cast<int32_t>(x.val);
    }

    void ensure_mem_word(size_t idx)
    {
        eassert(idx < 1024 * 1024 * 1024, "Insane memory access.");
        if (idx >= memory.size())
        {
            memory.resize(idx + 1, 0);
            std::cerr << "Memory expansion required by word index: " << idx << "\n";
        }
    }

    struct Inst
    {
        enum class Op : uint8_t
        {
            UNKNOWN = 0,
            ADD,
            ADDI,
            LUI,
            LW,
            LBU,
            SW,
            SB,
            JALR,
        } op = Op::UNKNOWN;


        uint32_t rd = 0;
        uint32_t rs1 = 0;
        uint32_t rs2 = 0;
        int32_t imm = 0;

        uint32_t encoding = 0;

        std::string dump(MiniRVEmu* emu) const
        {
            auto reg = [](uint32_t r) { return std::string("x") + std::to_string(r); };
            std::ostringstream ss;

            auto imm_str = [this]()
            {
                auto imm_u = static_cast<uint32_t>(imm);
                std::ostringstream tmp;
                tmp << imm << " (=0x" << std::hex << imm_u << std::dec << ")";
                return tmp.str();
            };

            std::vector<uint32_t> regs_to_dump;
            switch (op)
            {
            case Op::ADD:
                ss << "add " << reg(rd) << ", " << reg(rs1) << ", " << reg(rs2);
                regs_to_dump.emplace_back(rs1);
                regs_to_dump.emplace_back(rs2);
                break;

            case Op::ADDI:
                ss << "addi " << reg(rd) << ", " << reg(rs1) << ", " << imm_str();
                regs_to_dump.emplace_back(rs1);
                break;

            case Op::LUI:
                ss << "lui " << reg(rd) << ", " << imm_str();
                break;

            case Op::LW:
                ss << "lw " << reg(rd) << ", " << imm_str() << "(" << reg(rs1) << ")";
                regs_to_dump.emplace_back(rs1);
                break;

            case Op::LBU:
                ss << "lbu " << reg(rd) << ", " << imm_str() << "(" << reg(rs1) << ")";
                regs_to_dump.emplace_back(rs1);
                break;

            case Op::SW:
                ss << "sw " << reg(rs2) << ", " << imm_str() << "(" << reg(rs1) << ")";
                regs_to_dump.emplace_back(rs1);
                regs_to_dump.emplace_back(rs2);
                break;

            case Op::SB:
                ss << "sb " << reg(rs2) << ", " << imm_str() << "(" << reg(rs1) << ")";
                regs_to_dump.emplace_back(rs1);
                regs_to_dump.emplace_back(rs2);
                break;

            case Op::JALR:
                ss << "jalr " << reg(rd) << ", " << imm_str() << "(" << reg(rs1) << ")";
                regs_to_dump.emplace_back(rs1);
                break;

            case Op::UNKNOWN:
            default:
                ss << "unknown";
                break;
            }

            ss << " ; enc=0x" << std::hex << std::setw(8) << std::setfill('0') << encoding << std::dec;
            for (auto curr : regs_to_dump)
            {
                if (curr != 0)
                    ss << ", " << reg(curr) << "=0x" << std::hex << emu->regs[curr] << std::dec;
            }

            ss << ", addr" << "=0x" << std::hex << emu->pc * 4 << std::dec;

            return ss.str();
        }
    };

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
                inst.op = Inst::Op::ADD;
                return inst;
            }

        case 0b0010011:
            {
                // I-type (ADDI)
                eassert(funct3 == 0x0, "Bad inst");
                uint32_t imm12 = (encoding >> 20) & 0xfff;
                int32_t imm = sign_extend<12>(imm12);
                inst.imm = imm;
                inst.op = Inst::Op::ADDI;
                return inst;
            }
        case 0b0110111:
            {
                // LUI (U-type)
                uint32_t imm20 = encoding & 0xfffff000u;
                inst.imm = static_cast<int32_t>(imm20) >> 12;
                inst.op = Inst::Op::LUI;
                return inst;
            }
        case 0b0000011:
            {
                // Loads (I-type) LW, LBU ...
                uint32_t imm12 = (encoding >> 20) & 0xfff;
                inst.imm = sign_extend<12>(imm12);

                if (funct3 == 0b010)
                {
                    // LW
                    inst.op = Inst::Op::LW;
                    return inst;
                }

                if (funct3 == 0b100)
                {
                    // LBU
                    inst.op = Inst::Op::LBU;
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
                inst.imm = sign_extend<12>(imm12);

                if (funct3 == 0b010)
                {
                    // SW
                    inst.op = Inst::Op::SW;
                    return inst;
                }
                if (funct3 == 0)
                {
                    // SB
                    inst.op = Inst::Op::SB;
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
                inst.imm = sign_extend<12>(imm12);
                inst.op = Inst::Op::JALR;
                return inst;
            }

        default:
            eassert(false, "Bad inst");
            return inst;
        }
        return inst;
    }

    void execute(const Inst& inst)
    {
        switch (inst.op)
        {
        case Inst::Op::ADD:
            if (inst.rd != 0)
                regs[inst.rd] = regs[inst.rs1] + regs[inst.rs2];
            return;

        case Inst::Op::ADDI:
            if (inst.rd != 0)
                regs[inst.rd] = regs[inst.rs1] + static_cast<uint32_t>(inst.imm);
            return;

        case Inst::Op::LUI:
            if (inst.rd != 0)
                regs[inst.rd] = static_cast<uint32_t>(inst.imm << 12);
            return;
        case Inst::Op::LW:
            {
                uint32_t addr_bytes = regs[inst.rs1] + static_cast<uint32_t>(inst.imm);
                size_t word_idx = addr_bytes / 4;
                size_t byte_off = addr_bytes % 4;
                ensure_mem_word(word_idx);
                uint32_t word = memory[word_idx];
                eassert(byte_off == 0, "Unaligned access");
                if (inst.rd != 0)
                    regs[inst.rd] = word;

                // std::cerr << "Loaded 4 byte '" <<  word << "' into x" << inst.rd << std::endl;

                return;
            }

        case Inst::Op::LBU:
            {
                uint32_t addr_bytes = regs[inst.rs1] + static_cast<uint32_t>(inst.imm);
                size_t word_idx = addr_bytes / 4;
                size_t byte_off = addr_bytes % 4;
                ensure_mem_word(word_idx);
                uint32_t word = memory[word_idx];
                uint32_t b = (word >> (byte_off * 8)) & 0xffu;
                if (inst.rd != 0)
                    regs[inst.rd] = b;

                std::cerr << "Loaded 1 byte '" <<  b << "' into x" << inst.rd << std::endl;

                return;
            }


        case Inst::Op::SW:
            {
                uint32_t addr_bytes = regs[inst.rs1] + static_cast<uint32_t>(inst.imm);
                size_t word_idx = addr_bytes / 4;
                size_t byte_off = addr_bytes % 4;
                ensure_mem_word(word_idx);
                eassert(byte_off == 0, "Unaligned access");

                auto old_mem = memory[word_idx];

                memory[word_idx] = regs[inst.rs2];

                std::cerr << "4 byte changed at " << word_idx << " from " << old_mem << " to " << memory[word_idx] << std::endl;
                return;
            }


        case Inst::Op::SB:
            {
                uint32_t addr_bytes = regs[inst.rs1] + static_cast<uint32_t>(inst.imm);
                size_t word_idx = addr_bytes / 4;
                size_t byte_off = addr_bytes % 4;
                ensure_mem_word(word_idx);
                uint32_t old = memory[word_idx];
                uint32_t mask = ~(0xffu << (byte_off * 8));
                uint32_t newval = (old & mask) | ((regs[inst.rs2] & 0xffu) << (byte_off * 8));
                memory[word_idx] = newval;

                // std::cerr << "1 byte changed at " << word_idx << " from " << old << " to " << memory[word_idx] << std::endl;
                return;
            }


        case Inst::Op::JALR:
            {
                uint32_t old_pc = pc;

                uint32_t target = (regs[inst.rs1] + static_cast<uint32_t>(inst.imm)) & ~1u;
                uint32_t new_pc_word = target / 4u;
                pc = new_pc_word - 1u; // run loop will pc++ after execute

                uint32_t return_addr_bytes = (old_pc + 1u) * 4u;
                if (inst.rd != 0)
                    regs[inst.rd] = return_addr_bytes;
                return;
            }


        case Inst::Op::UNKNOWN:
        default:
            eassert(false, "Cannot execute unknown op");
            return;
        }
    }
};
#endif