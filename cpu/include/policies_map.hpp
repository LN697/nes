#pragma once

#include "core_policies.hpp"

// ============================================================================
// CYCLE-ACCURATE BRANCH HELPER
// ============================================================================
// Branches are unique: 2 cycles (base), +1 if taken, +2 if page crossed.
template <typename CONDITION>
static void ExecBranchCycles(Core& core) {
    uint16_t pc = static_cast<uint16_t>(core.pc.getValue());
    int8_t offset = static_cast<int8_t>(core.fetch()); // Fetch operand (PC++)
    
    // Base: 2 Cycles (Fetch Opcode + Fetch Operand)
    core.last_cycles = 2; 

    if (CONDITION::check(core)) {
        core.last_cycles++; // +1 Cycle for taking the branch
        
        uint16_t target = pc + 1 + offset;
        
        // +1 Cycle if page boundary crossed
        // (High byte of PC+1 != High byte of Target)
        if (((pc + 1) & 0xFF00) != (target & 0xFF00)) {
             core.last_cycles++;
        }
        
        core.pc.setValue(target);
    }
}

// ============================================================================
// INSTRUCTION TABLE
// ============================================================================
static void init_instr_table(Core &core) {
    core.instr_table.fill(nullptr);
    
    // --- Column 0 (Control/Stack/Imm) ---
    // BRK: 7 cycles
    core.instr_table[0x00] = +[](Core& c){ Op_BRK::exec(c, 0); c.last_cycles = 7; };
    // Branches: 2+ cycles (handled by helper)
    core.instr_table[0x10] = +[](Core& c){ ExecBranchCycles<Cond_BPL>(c); };
    // JSR: 6 cycles
    core.instr_table[0x20] = +[](Core& c){ ExecJSR(c); c.last_cycles = 6; };
    core.instr_table[0x30] = +[](Core& c){ ExecBranchCycles<Cond_BMI>(c); };
    // RTI: 6 cycles
    core.instr_table[0x40] = +[](Core& c){ Op_RTI::exec(c, 0); c.last_cycles = 6; };
    core.instr_table[0x50] = +[](Core& c){ ExecBranchCycles<Cond_BVC>(c); };
    // RTS: 6 cycles
    core.instr_table[0x60] = +[](Core& c){ Op_RTS::exec(c, 0); c.last_cycles = 6; };
    core.instr_table[0x70] = +[](Core& c){ ExecBranchCycles<Cond_BVS>(c); };
    // NOP #imm: 2 bytes, 2 cycles
    core.instr_table[0x80] = +[](Core& c){ ExecRead<IMM, Op_NOP>(c); c.last_cycles = 2; };
    core.instr_table[0x90] = +[](Core& c){ ExecBranchCycles<Cond_BCC>(c); };
    // LDY #imm: 2 cycles
    core.instr_table[0xA0] = +[](Core& c){ ExecRead<IMM, Op_LDY>(c); c.last_cycles = 2; };
    core.instr_table[0xB0] = +[](Core& c){ ExecBranchCycles<Cond_BCS>(c); };
    // CPY #imm: 2 cycles
    core.instr_table[0xC0] = +[](Core& c){ ExecRead<IMM, Op_CPY>(c); c.last_cycles = 2; };
    core.instr_table[0xD0] = +[](Core& c){ ExecBranchCycles<Cond_BNE>(c); };
    // CPX #imm: 2 cycles
    core.instr_table[0xE0] = +[](Core& c){ ExecRead<IMM, Op_CPX>(c); c.last_cycles = 2; };
    core.instr_table[0xF0] = +[](Core& c){ ExecBranchCycles<Cond_BEQ>(c); };

    // --- Column 1 (Indirect X / Indirect Y) ---
    // (Indirect, X) is always 6 cycles.
    // (Indirect), Y is 5 cycles (+1 if page crossed). We set base 5.
    
    core.instr_table[0x01] = +[](Core& c){ ExecRead<INDX, Op_ORA>(c); c.last_cycles = 6; };
    core.instr_table[0x11] = +[](Core& c){ ExecRead<INDY, Op_ORA>(c); c.last_cycles = 5; };
    core.instr_table[0x21] = +[](Core& c){ ExecRead<INDX, Op_AND>(c); c.last_cycles = 6; };
    core.instr_table[0x31] = +[](Core& c){ ExecRead<INDY, Op_AND>(c); c.last_cycles = 5; };
    core.instr_table[0x41] = +[](Core& c){ ExecRead<INDX, Op_EOR>(c); c.last_cycles = 6; };
    core.instr_table[0x51] = +[](Core& c){ ExecRead<INDY, Op_EOR>(c); c.last_cycles = 5; };
    core.instr_table[0x61] = +[](Core& c){ ExecRead<INDX, Op_ADC>(c); c.last_cycles = 6; };
    core.instr_table[0x71] = +[](Core& c){ ExecRead<INDY, Op_ADC>(c); c.last_cycles = 5; };
    // STA (Ind),Y is always 6 cycles (due to write fixup)
    core.instr_table[0x81] = +[](Core& c){ ExecWrite<INDX, Src_A>(c); c.last_cycles = 6; };
    core.instr_table[0x91] = +[](Core& c){ ExecWrite<INDY, Src_A>(c); c.last_cycles = 6; };
    core.instr_table[0xA1] = +[](Core& c){ ExecRead<INDX, Op_LDA>(c); c.last_cycles = 6; };
    core.instr_table[0xB1] = +[](Core& c){ ExecRead<INDY, Op_LDA>(c); c.last_cycles = 5; };
    core.instr_table[0xC1] = +[](Core& c){ ExecRead<INDX, Op_CMP>(c); c.last_cycles = 6; };
    core.instr_table[0xD1] = +[](Core& c){ ExecRead<INDY, Op_CMP>(c); c.last_cycles = 5; };
    core.instr_table[0xE1] = +[](Core& c){ ExecRead<INDX, Op_SBC>(c); c.last_cycles = 6; };
    core.instr_table[0xF1] = +[](Core& c){ ExecRead<INDY, Op_SBC>(c); c.last_cycles = 5; };

    // --- Column 2 (JAM / Illegal NOP / LDX Imm) ---
    // JAM loops forever, cycles effectively infinite, but per step usually 2.
    auto JAM = +[](Core& c){ Op_JAM::exec(c, 0); c.last_cycles = 2; };
    core.instr_table[0x02] = JAM;
    core.instr_table[0x12] = JAM;
    core.instr_table[0x22] = JAM;
    core.instr_table[0x32] = JAM;
    core.instr_table[0x42] = JAM;
    core.instr_table[0x52] = JAM;
    core.instr_table[0x62] = JAM;
    core.instr_table[0x72] = JAM;
    // NOP #imm: 2 bytes, 2 cycles
    core.instr_table[0x82] = +[](Core& c){ ExecRead<IMM, Op_NOP>(c); c.last_cycles = 2; };
    core.instr_table[0x92] = JAM;
    // LDX #imm: 2 cycles
    core.instr_table[0xA2] = +[](Core& c){ ExecRead<IMM, Op_LDX>(c); c.last_cycles = 2; };
    core.instr_table[0xB2] = JAM;
    core.instr_table[0xC2] = +[](Core& c){ ExecRead<IMM, Op_NOP>(c); c.last_cycles = 2; };
    core.instr_table[0xD2] = JAM;
    core.instr_table[0xE2] = +[](Core& c){ ExecRead<IMM, Op_NOP>(c); c.last_cycles = 2; };
    core.instr_table[0xF2] = JAM;

    // --- Column 3 (Illegal RMW+ALU Combos) ---
    // Ind,X is usually 8 cycles for RMW. Ind,Y is 8.
    core.instr_table[0x03] = +[](Core& c){ ExecRMW_ALU<INDX, Op_ASL, Op_ORA>(c); c.last_cycles = 8; };
    core.instr_table[0x13] = +[](Core& c){ ExecRMW_ALU<INDY, Op_ASL, Op_ORA>(c); c.last_cycles = 8; };
    core.instr_table[0x23] = +[](Core& c){ ExecRMW_ALU<INDX, Op_ROL, Op_AND>(c); c.last_cycles = 8; };
    core.instr_table[0x33] = +[](Core& c){ ExecRMW_ALU<INDY, Op_ROL, Op_AND>(c); c.last_cycles = 8; };
    core.instr_table[0x43] = +[](Core& c){ ExecRMW_ALU<INDX, Op_LSR, Op_EOR>(c); c.last_cycles = 8; };
    core.instr_table[0x53] = +[](Core& c){ ExecRMW_ALU<INDY, Op_LSR, Op_EOR>(c); c.last_cycles = 8; };
    core.instr_table[0x63] = +[](Core& c){ ExecRMW_ALU<INDX, Op_ROR, Op_ADC>(c); c.last_cycles = 8; };
    core.instr_table[0x73] = +[](Core& c){ ExecRMW_ALU<INDY, Op_ROR, Op_ADC>(c); c.last_cycles = 8; };
    // SAX Ind,X: 6 cycles
    core.instr_table[0x83] = +[](Core& c){ ExecWrite<INDX, Src_AX>(c); c.last_cycles = 6; };
    // SHA Ind,Y: 6 cycles
    core.instr_table[0x93] = +[](Core& c){ ExecSHA_INDY(c); c.last_cycles = 6; };
    // LAX Ind,X: 6 cycles. Ind,Y: 5 cycles
    core.instr_table[0xA3] = +[](Core& c){ ExecRead<INDX, Op_LAX>(c); c.last_cycles = 6; };
    core.instr_table[0xB3] = +[](Core& c){ ExecRead<INDY, Op_LAX>(c); c.last_cycles = 5; };
    // DCP/ISB: 8 cycles
    core.instr_table[0xC3] = +[](Core& c){ ExecRMW_ALU<INDX, Op_DEC, Op_CMP>(c); c.last_cycles = 8; };
    core.instr_table[0xD3] = +[](Core& c){ ExecRMW_ALU<INDY, Op_DEC, Op_CMP>(c); c.last_cycles = 8; };
    core.instr_table[0xE3] = +[](Core& c){ ExecRMW_ALU<INDX, Op_INC, Op_SBC>(c); c.last_cycles = 8; };
    core.instr_table[0xF3] = +[](Core& c){ ExecRMW_ALU<INDY, Op_INC, Op_SBC>(c); c.last_cycles = 8; };

    // --- Column 4 (Zero Page) ---
    // NOP ZP: 3 cycles
    core.instr_table[0x04] = +[](Core& c){ ExecRead<ZP, Op_NOP>(c); c.last_cycles = 3; };
    core.instr_table[0x14] = +[](Core& c){ ExecRead<ZPX, Op_NOP>(c); c.last_cycles = 4; };
    core.instr_table[0x24] = +[](Core& c){ ExecRead<ZP, Op_BIT>(c); c.last_cycles = 3; };
    core.instr_table[0x34] = +[](Core& c){ ExecRead<ZPX, Op_NOP>(c); c.last_cycles = 4; };
    core.instr_table[0x44] = +[](Core& c){ ExecRead<ZP, Op_NOP>(c); c.last_cycles = 3; };
    core.instr_table[0x54] = +[](Core& c){ ExecRead<ZPX, Op_NOP>(c); c.last_cycles = 4; };
    core.instr_table[0x64] = +[](Core& c){ ExecRead<ZP, Op_NOP>(c); c.last_cycles = 3; };
    core.instr_table[0x74] = +[](Core& c){ ExecRead<ZPX, Op_NOP>(c); c.last_cycles = 4; };
    // STY ZP: 3 cycles. STY ZP,X: 4 cycles
    core.instr_table[0x84] = +[](Core& c){ ExecWrite<ZP, Src_Y>(c); c.last_cycles = 3; };
    core.instr_table[0x94] = +[](Core& c){ ExecWrite<ZPX, Src_Y>(c); c.last_cycles = 4; };
    // LDY ZP: 3 cycles. LDY ZP,X: 4 cycles
    core.instr_table[0xA4] = +[](Core& c){ ExecRead<ZP, Op_LDY>(c); c.last_cycles = 3; };
    core.instr_table[0xB4] = +[](Core& c){ ExecRead<ZPX, Op_LDY>(c); c.last_cycles = 4; };
    // CPY ZP: 3 cycles
    core.instr_table[0xC4] = +[](Core& c){ ExecRead<ZP, Op_CPY>(c); c.last_cycles = 3; };
    core.instr_table[0xD4] = +[](Core& c){ ExecRead<ZPX, Op_NOP>(c); c.last_cycles = 4; };
    // CPX ZP: 3 cycles
    core.instr_table[0xE4] = +[](Core& c){ ExecRead<ZP, Op_CPX>(c); c.last_cycles = 3; };
    core.instr_table[0xF4] = +[](Core& c){ ExecRead<ZPX, Op_NOP>(c); c.last_cycles = 4; };

    // --- Column 5 (Zero Page ALU) ---
    // ZP: 3 cycles. ZP,X: 4 cycles.
    core.instr_table[0x05] = +[](Core& c){ ExecRead<ZP, Op_ORA>(c); c.last_cycles = 3; };
    core.instr_table[0x15] = +[](Core& c){ ExecRead<ZPX, Op_ORA>(c); c.last_cycles = 4; };
    core.instr_table[0x25] = +[](Core& c){ ExecRead<ZP, Op_AND>(c); c.last_cycles = 3; };
    core.instr_table[0x35] = +[](Core& c){ ExecRead<ZPX, Op_AND>(c); c.last_cycles = 4; };
    core.instr_table[0x45] = +[](Core& c){ ExecRead<ZP, Op_EOR>(c); c.last_cycles = 3; };
    core.instr_table[0x55] = +[](Core& c){ ExecRead<ZPX, Op_EOR>(c); c.last_cycles = 4; };
    core.instr_table[0x65] = +[](Core& c){ ExecRead<ZP, Op_ADC>(c); c.last_cycles = 3; };
    core.instr_table[0x75] = +[](Core& c){ ExecRead<ZPX, Op_ADC>(c); c.last_cycles = 4; };
    core.instr_table[0x85] = +[](Core& c){ ExecWrite<ZP, Src_A>(c); c.last_cycles = 3; };
    core.instr_table[0x95] = +[](Core& c){ ExecWrite<ZPX, Src_A>(c); c.last_cycles = 4; };
    core.instr_table[0xA5] = +[](Core& c){ ExecRead<ZP, Op_LDA>(c); c.last_cycles = 3; };
    core.instr_table[0xB5] = +[](Core& c){ ExecRead<ZPX, Op_LDA>(c); c.last_cycles = 4; };
    core.instr_table[0xC5] = +[](Core& c){ ExecRead<ZP, Op_CMP>(c); c.last_cycles = 3; };
    core.instr_table[0xD5] = +[](Core& c){ ExecRead<ZPX, Op_CMP>(c); c.last_cycles = 4; };
    core.instr_table[0xE5] = +[](Core& c){ ExecRead<ZP, Op_SBC>(c); c.last_cycles = 3; };
    core.instr_table[0xF5] = +[](Core& c){ ExecRead<ZPX, Op_SBC>(c); c.last_cycles = 4; };

    // --- Column 6 (Zero Page RMW) ---
    // ZP RMW: 5 cycles. ZP,X RMW: 6 cycles.
    core.instr_table[0x06] = +[](Core& c){ ExecRMW<ZP, Op_ASL>(c); c.last_cycles = 5; };
    core.instr_table[0x16] = +[](Core& c){ ExecRMW<ZPX, Op_ASL>(c); c.last_cycles = 6; };
    core.instr_table[0x26] = +[](Core& c){ ExecRMW<ZP, Op_ROL>(c); c.last_cycles = 5; };
    core.instr_table[0x36] = +[](Core& c){ ExecRMW<ZPX, Op_ROL>(c); c.last_cycles = 6; };
    core.instr_table[0x46] = +[](Core& c){ ExecRMW<ZP, Op_LSR>(c); c.last_cycles = 5; };
    core.instr_table[0x56] = +[](Core& c){ ExecRMW<ZPX, Op_LSR>(c); c.last_cycles = 6; };
    core.instr_table[0x66] = +[](Core& c){ ExecRMW<ZP, Op_ROR>(c); c.last_cycles = 5; };
    core.instr_table[0x76] = +[](Core& c){ ExecRMW<ZPX, Op_ROR>(c); c.last_cycles = 6; };
    // STX ZP: 3. STX ZP,Y: 4.
    core.instr_table[0x86] = +[](Core& c){ ExecWrite<ZP, Src_X>(c); c.last_cycles = 3; };
    core.instr_table[0x96] = +[](Core& c){ ExecWrite<ZPY, Src_X>(c); c.last_cycles = 4; };
    // LDX ZP: 3. LDX ZP,Y: 4.
    core.instr_table[0xA6] = +[](Core& c){ ExecRead<ZP, Op_LDX>(c); c.last_cycles = 3; };
    core.instr_table[0xB6] = +[](Core& c){ ExecRead<ZPY, Op_LDX>(c); c.last_cycles = 4; };
    // DEC/INC ZP: 5. ZP,X: 6.
    core.instr_table[0xC6] = +[](Core& c){ ExecRMW<ZP, Op_DEC>(c); c.last_cycles = 5; };
    core.instr_table[0xD6] = +[](Core& c){ ExecRMW<ZPX, Op_DEC>(c); c.last_cycles = 6; };
    core.instr_table[0xE6] = +[](Core& c){ ExecRMW<ZP, Op_INC>(c); c.last_cycles = 5; };
    core.instr_table[0xF6] = +[](Core& c){ ExecRMW<ZPX, Op_INC>(c); c.last_cycles = 6; };

    // --- Column 7 (Illegal ZP RMW+ALU Combos) ---
    // ZP Combos: 5 cycles. ZP,X Combos: 6 cycles.
    core.instr_table[0x07] = +[](Core& c){ ExecRMW_ALU<ZP, Op_ASL, Op_ORA>(c); c.last_cycles = 5; };
    core.instr_table[0x17] = +[](Core& c){ ExecRMW_ALU<ZPX, Op_ASL, Op_ORA>(c); c.last_cycles = 6; };
    core.instr_table[0x27] = +[](Core& c){ ExecRMW_ALU<ZP, Op_ROL, Op_AND>(c); c.last_cycles = 5; };
    core.instr_table[0x37] = +[](Core& c){ ExecRMW_ALU<ZPX, Op_ROL, Op_AND>(c); c.last_cycles = 6; };
    core.instr_table[0x47] = +[](Core& c){ ExecRMW_ALU<ZP, Op_LSR, Op_EOR>(c); c.last_cycles = 5; };
    core.instr_table[0x57] = +[](Core& c){ ExecRMW_ALU<ZPX, Op_LSR, Op_EOR>(c); c.last_cycles = 6; };
    core.instr_table[0x67] = +[](Core& c){ ExecRMW_ALU<ZP, Op_ROR, Op_ADC>(c); c.last_cycles = 5; };
    core.instr_table[0x77] = +[](Core& c){ ExecRMW_ALU<ZPX, Op_ROR, Op_ADC>(c); c.last_cycles = 6; };
    // SAX ZP: 3. SAX ZP,Y: 4.
    core.instr_table[0x87] = +[](Core& c){ ExecWrite<ZP, Src_AX>(c); c.last_cycles = 3; };
    core.instr_table[0x97] = +[](Core& c){ ExecWrite<ZPY, Src_AX>(c); c.last_cycles = 4; };
    // LAX ZP: 3. LAX ZP,Y: 4.
    core.instr_table[0xA7] = +[](Core& c){ ExecRead<ZP, Op_LAX>(c); c.last_cycles = 3; };
    core.instr_table[0xB7] = +[](Core& c){ ExecRead<ZPY, Op_LAX>(c); c.last_cycles = 4; };
    // DCP/ISB ZP: 5. ZP,X: 6.
    core.instr_table[0xC7] = +[](Core& c){ ExecRMW_ALU<ZP, Op_DEC, Op_CMP>(c); c.last_cycles = 5; };
    core.instr_table[0xD7] = +[](Core& c){ ExecRMW_ALU<ZPX, Op_DEC, Op_CMP>(c); c.last_cycles = 6; };
    core.instr_table[0xE7] = +[](Core& c){ ExecRMW_ALU<ZP, Op_INC, Op_SBC>(c); c.last_cycles = 5; };
    core.instr_table[0xF7] = +[](Core& c){ ExecRMW_ALU<ZPX, Op_INC, Op_SBC>(c); c.last_cycles = 6; };

    // --- Column 8 (Implied) ---
    // All 2 cycles, except Pushes (PHA/PHP 3) and Pulls (PLA/PLP 4)
    core.instr_table[0x08] = +[](Core& c){ Op_PHP::exec(c, 0); c.last_cycles = 3; };
    core.instr_table[0x18] = +[](Core& c){ Op_CLC::exec(c, 0); c.last_cycles = 2; };
    core.instr_table[0x28] = +[](Core& c){ Op_PLP::exec(c, 0); c.last_cycles = 4; };
    core.instr_table[0x38] = +[](Core& c){ Op_SEC::exec(c, 0); c.last_cycles = 2; };
    core.instr_table[0x48] = +[](Core& c){ Op_PHA::exec(c, 0); c.last_cycles = 3; };
    core.instr_table[0x58] = +[](Core& c){ Op_CLI::exec(c, 0); c.last_cycles = 2; };
    core.instr_table[0x68] = +[](Core& c){ Op_PLA::exec(c, 0); c.last_cycles = 4; };
    core.instr_table[0x78] = +[](Core& c){ Op_SEI::exec(c, 0); c.last_cycles = 2; };
    core.instr_table[0x88] = +[](Core& c){ Op_DEY::exec(c, 0); c.last_cycles = 2; };
    core.instr_table[0x98] = +[](Core& c){ Op_TYA::exec(c, 0); c.last_cycles = 2; };
    core.instr_table[0xA8] = +[](Core& c){ Op_TAY::exec(c, 0); c.last_cycles = 2; };
    core.instr_table[0xB8] = +[](Core& c){ Op_CLV::exec(c, 0); c.last_cycles = 2; };
    core.instr_table[0xC8] = +[](Core& c){ Op_INY::exec(c, 0); c.last_cycles = 2; };
    core.instr_table[0xD8] = +[](Core& c){ Op_CLD::exec(c, 0); c.last_cycles = 2; };
    core.instr_table[0xE8] = +[](Core& c){ Op_INX::exec(c, 0); c.last_cycles = 2; };
    core.instr_table[0xF8] = +[](Core& c){ Op_SED::exec(c, 0); c.last_cycles = 2; };

    // --- Column 9 (Immediate / Absolute Y) ---
    // Imm: 2 cycles. Abs, Y: 4 cycles (+1 if page crossed).
    core.instr_table[0x09] = +[](Core& c){ ExecRead<IMM, Op_ORA>(c); c.last_cycles = 2; };
    core.instr_table[0x19] = +[](Core& c){ ExecRead<ABSY, Op_ORA>(c); c.last_cycles = 4; };
    core.instr_table[0x29] = +[](Core& c){ ExecRead<IMM, Op_AND>(c); c.last_cycles = 2; };
    core.instr_table[0x39] = +[](Core& c){ ExecRead<ABSY, Op_AND>(c); c.last_cycles = 4; };
    core.instr_table[0x49] = +[](Core& c){ ExecRead<IMM, Op_EOR>(c); c.last_cycles = 2; };
    core.instr_table[0x59] = +[](Core& c){ ExecRead<ABSY, Op_EOR>(c); c.last_cycles = 4; };
    core.instr_table[0x69] = +[](Core& c){ ExecRead<IMM, Op_ADC>(c); c.last_cycles = 2; };
    core.instr_table[0x79] = +[](Core& c){ ExecRead<ABSY, Op_ADC>(c); c.last_cycles = 4; };
    core.instr_table[0x89] = +[](Core& c){ ExecRead<IMM, Op_NOP>(c); c.last_cycles = 2; };
    // STA Abs,Y: 5 cycles (always)
    core.instr_table[0x99] = +[](Core& c){ ExecWrite<ABSY, Src_A>(c); c.last_cycles = 5; };
    core.instr_table[0xA9] = +[](Core& c){ ExecRead<IMM, Op_LDA>(c); c.last_cycles = 2; };
    core.instr_table[0xB9] = +[](Core& c){ ExecRead<ABSY, Op_LDA>(c); c.last_cycles = 4; };
    core.instr_table[0xC9] = +[](Core& c){ ExecRead<IMM, Op_CMP>(c); c.last_cycles = 2; };
    core.instr_table[0xD9] = +[](Core& c){ ExecRead<ABSY, Op_CMP>(c); c.last_cycles = 4; };
    core.instr_table[0xE9] = +[](Core& c){ ExecRead<IMM, Op_SBC>(c); c.last_cycles = 2; };
    core.instr_table[0xF9] = +[](Core& c){ ExecRead<ABSY, Op_SBC>(c); c.last_cycles = 4; };

    // --- Column A (Accumulator / Implied) ---
    // Shift A: 2 cycles. Implied NOPs: 2 cycles.
    core.instr_table[0x0A] = +[](Core& c){ ExecRMW<ACC, Op_ASL>(c); c.last_cycles = 2; };
    core.instr_table[0x1A] = +[](Core& c){ Op_NOP::exec(c, 0); c.last_cycles = 2; };
    core.instr_table[0x2A] = +[](Core& c){ ExecRMW<ACC, Op_ROL>(c); c.last_cycles = 2; };
    core.instr_table[0x3A] = +[](Core& c){ Op_NOP::exec(c, 0); c.last_cycles = 2; };
    core.instr_table[0x4A] = +[](Core& c){ ExecRMW<ACC, Op_LSR>(c); c.last_cycles = 2; };
    core.instr_table[0x5A] = +[](Core& c){ Op_NOP::exec(c, 0); c.last_cycles = 2; };
    core.instr_table[0x6A] = +[](Core& c){ ExecRMW<ACC, Op_ROR>(c); c.last_cycles = 2; };
    core.instr_table[0x7A] = +[](Core& c){ Op_NOP::exec(c, 0); c.last_cycles = 2; };
    // Transfers: 2 cycles
    core.instr_table[0x8A] = +[](Core& c){ Op_TXA::exec(c, 0); c.last_cycles = 2; };
    core.instr_table[0x9A] = +[](Core& c){ Op_TXS::exec(c, 0); c.last_cycles = 2; };
    core.instr_table[0xAA] = +[](Core& c){ Op_TAX::exec(c, 0); c.last_cycles = 2; };
    core.instr_table[0xBA] = +[](Core& c){ Op_TSX::exec(c, 0); c.last_cycles = 2; };
    core.instr_table[0xCA] = +[](Core& c){ Op_DEX::exec(c, 0); c.last_cycles = 2; };
    core.instr_table[0xDA] = +[](Core& c){ Op_NOP::exec(c, 0); c.last_cycles = 2; };
    core.instr_table[0xEA] = +[](Core& c){ Op_NOP::exec(c, 0); c.last_cycles = 2; };
    core.instr_table[0xFA] = +[](Core& c){ Op_NOP::exec(c, 0); c.last_cycles = 2; };

    // --- Column B (Immediate Illegal / Abs Y Combos) ---
    // Imm: 2. Abs,Y RMW: 7.
    core.instr_table[0x0B] = +[](Core& c){ ExecRead<IMM, Op_ANC>(c); c.last_cycles = 2; };
    core.instr_table[0x1B] = +[](Core& c){ ExecRMW_ALU<ABSY, Op_ASL, Op_ORA>(c); c.last_cycles = 7; };
    core.instr_table[0x2B] = +[](Core& c){ ExecRead<IMM, Op_ANC>(c); c.last_cycles = 2; };
    core.instr_table[0x3B] = +[](Core& c){ ExecRMW_ALU<ABSY, Op_ROL, Op_AND>(c); c.last_cycles = 7; };
    core.instr_table[0x4B] = +[](Core& c){ ExecRead<IMM, Op_ALR>(c); c.last_cycles = 2; };
    core.instr_table[0x5B] = +[](Core& c){ ExecRMW_ALU<ABSY, Op_LSR, Op_EOR>(c); c.last_cycles = 7; };
    core.instr_table[0x6B] = +[](Core& c){ ExecRead<IMM, Op_ARR>(c); c.last_cycles = 2; };
    core.instr_table[0x7B] = +[](Core& c){ ExecRMW_ALU<ABSY, Op_ROR, Op_ADC>(c); c.last_cycles = 7; };
    core.instr_table[0x8B] = +[](Core& c){ ExecRead<IMM, Op_XAA>(c); c.last_cycles = 2; };
    // TAS: 5
    core.instr_table[0x9B] = +[](Core& c){ ExecTAS(c); c.last_cycles = 5; };
    // LAX Imm: 2
    core.instr_table[0xAB] = +[](Core& c){ ExecRead<IMM, Op_ATX>(c); c.last_cycles = 2; };
    // LAS: 4 (+1)
    core.instr_table[0xBB] = +[](Core& c){ ExecRead<ABSY, Op_LAS>(c); c.last_cycles = 4; };
    core.instr_table[0xCB] = +[](Core& c){ ExecRead<IMM, Op_AXS>(c); c.last_cycles = 2; };
    core.instr_table[0xDB] = +[](Core& c){ ExecRMW_ALU<ABSY, Op_DEC, Op_CMP>(c); c.last_cycles = 7; };
    core.instr_table[0xEB] = +[](Core& c){ ExecRead<IMM, Op_SBC>(c); c.last_cycles = 2; };
    core.instr_table[0xFB] = +[](Core& c){ ExecRMW_ALU<ABSY, Op_INC, Op_SBC>(c); c.last_cycles = 7; };

    // --- Column C (Absolute / Abs X) ---
    // NOP Abs: 4. Abs,X: 4.
    core.instr_table[0x0C] = +[](Core& c){ ExecRead<ABS, Op_NOP>(c); c.last_cycles = 4; };
    core.instr_table[0x1C] = +[](Core& c){ ExecRead<ABSX, Op_NOP>(c); c.last_cycles = 4; };
    core.instr_table[0x2C] = +[](Core& c){ ExecRead<ABS, Op_BIT>(c); c.last_cycles = 4; };
    core.instr_table[0x3C] = +[](Core& c){ ExecRead<ABSX, Op_NOP>(c); c.last_cycles = 4; };
    // JMP Abs: 3
    core.instr_table[0x4C] = +[](Core& c){ ExecJMP<ABS>(c); c.last_cycles = 3; };
    core.instr_table[0x5C] = +[](Core& c){ ExecRead<ABSX, Op_NOP>(c); c.last_cycles = 4; };
    // JMP Ind: 5
    core.instr_table[0x6C] = +[](Core& c){ ExecJMP<IND>(c); c.last_cycles = 5; };
    core.instr_table[0x7C] = +[](Core& c){ ExecRead<ABSX, Op_NOP>(c); c.last_cycles = 4; };
    // STY Abs: 4. SHY: 5
    core.instr_table[0x8C] = +[](Core& c){ ExecWrite<ABS, Src_Y>(c); c.last_cycles = 4; };
    core.instr_table[0x9C] = +[](Core& c){ ExecSHY(c); c.last_cycles = 5; };
    // LDY Abs: 4. LDY Abs,X: 4
    core.instr_table[0xAC] = +[](Core& c){ ExecRead<ABS, Op_LDY>(c); c.last_cycles = 4; };
    core.instr_table[0xBC] = +[](Core& c){ ExecRead<ABSX, Op_LDY>(c); c.last_cycles = 4; };
    // CPY Abs: 4
    core.instr_table[0xCC] = +[](Core& c){ ExecRead<ABS, Op_CPY>(c); c.last_cycles = 4; };
    core.instr_table[0xDC] = +[](Core& c){ ExecRead<ABSX, Op_NOP>(c); c.last_cycles = 4; };
    // CPX Abs: 4
    core.instr_table[0xEC] = +[](Core& c){ ExecRead<ABS, Op_CPX>(c); c.last_cycles = 4; };
    core.instr_table[0xFC] = +[](Core& c){ ExecRead<ABSX, Op_NOP>(c); c.last_cycles = 4; };

    // --- Column D (Abs X ALU) ---
    // Abs: 4. Abs,X: 4.
    core.instr_table[0x0D] = +[](Core& c){ ExecRead<ABS, Op_ORA>(c); c.last_cycles = 4; };
    core.instr_table[0x1D] = +[](Core& c){ ExecRead<ABSX, Op_ORA>(c); c.last_cycles = 4; };
    core.instr_table[0x2D] = +[](Core& c){ ExecRead<ABS, Op_AND>(c); c.last_cycles = 4; };
    core.instr_table[0x3D] = +[](Core& c){ ExecRead<ABSX, Op_AND>(c); c.last_cycles = 4; };
    core.instr_table[0x4D] = +[](Core& c){ ExecRead<ABS, Op_EOR>(c); c.last_cycles = 4; };
    core.instr_table[0x5D] = +[](Core& c){ ExecRead<ABSX, Op_EOR>(c); c.last_cycles = 4; };
    core.instr_table[0x6D] = +[](Core& c){ ExecRead<ABS, Op_ADC>(c); c.last_cycles = 4; };
    core.instr_table[0x7D] = +[](Core& c){ ExecRead<ABSX, Op_ADC>(c); c.last_cycles = 4; };
    // STA Abs: 4. STA Abs,X: 5 (always)
    core.instr_table[0x8D] = +[](Core& c){ ExecWrite<ABS, Src_A>(c); c.last_cycles = 4; };
    core.instr_table[0x9D] = +[](Core& c){ ExecWrite<ABSX, Src_A>(c); c.last_cycles = 5; };
    core.instr_table[0xAD] = +[](Core& c){ ExecRead<ABS, Op_LDA>(c); c.last_cycles = 4; };
    core.instr_table[0xBD] = +[](Core& c){ ExecRead<ABSX, Op_LDA>(c); c.last_cycles = 4; };
    core.instr_table[0xCD] = +[](Core& c){ ExecRead<ABS, Op_CMP>(c); c.last_cycles = 4; };
    core.instr_table[0xDD] = +[](Core& c){ ExecRead<ABSX, Op_CMP>(c); c.last_cycles = 4; };
    core.instr_table[0xED] = +[](Core& c){ ExecRead<ABS, Op_SBC>(c); c.last_cycles = 4; };
    core.instr_table[0xFD] = +[](Core& c){ ExecRead<ABSX, Op_SBC>(c); c.last_cycles = 4; };

    // --- Column E (Abs RMW) ---
    // Abs RMW: 6. Abs,X RMW: 7.
    core.instr_table[0x0E] = +[](Core& c){ ExecRMW<ABS, Op_ASL>(c); c.last_cycles = 6; };
    core.instr_table[0x1E] = +[](Core& c){ ExecRMW<ABSX, Op_ASL>(c); c.last_cycles = 7; };
    core.instr_table[0x2E] = +[](Core& c){ ExecRMW<ABS, Op_ROL>(c); c.last_cycles = 6; };
    core.instr_table[0x3E] = +[](Core& c){ ExecRMW<ABSX, Op_ROL>(c); c.last_cycles = 7; };
    core.instr_table[0x4E] = +[](Core& c){ ExecRMW<ABS, Op_LSR>(c); c.last_cycles = 6; };
    core.instr_table[0x5E] = +[](Core& c){ ExecRMW<ABSX, Op_LSR>(c); c.last_cycles = 7; };
    core.instr_table[0x6E] = +[](Core& c){ ExecRMW<ABS, Op_ROR>(c); c.last_cycles = 6; };
    core.instr_table[0x7E] = +[](Core& c){ ExecRMW<ABSX, Op_ROR>(c); c.last_cycles = 7; };
    // STX Abs: 4. SHX: 5.
    core.instr_table[0x8E] = +[](Core& c){ ExecWrite<ABS, Src_X>(c); c.last_cycles = 4; };
    core.instr_table[0x9E] = +[](Core& c){ ExecSHX(c); c.last_cycles = 5; };
    // LDX Abs: 4. LDX Abs,Y: 4.
    core.instr_table[0xAE] = +[](Core& c){ ExecRead<ABS, Op_LDX>(c); c.last_cycles = 4; };
    core.instr_table[0xBE] = +[](Core& c){ ExecRead<ABSY, Op_LDX>(c); c.last_cycles = 4; };
    // DEC/INC Abs: 6. Abs,X: 7.
    core.instr_table[0xCE] = +[](Core& c){ ExecRMW<ABS, Op_DEC>(c); c.last_cycles = 6; };
    core.instr_table[0xDE] = +[](Core& c){ ExecRMW<ABSX, Op_DEC>(c); c.last_cycles = 7; };
    core.instr_table[0xEE] = +[](Core& c){ ExecRMW<ABS, Op_INC>(c); c.last_cycles = 6; };
    core.instr_table[0xFE] = +[](Core& c){ ExecRMW<ABSX, Op_INC>(c); c.last_cycles = 7; };

    // --- Column F (Abs Combos) ---
    // Abs Combos: 6. Abs,X Combos: 7.
    core.instr_table[0x0F] = +[](Core& c){ ExecRMW_ALU<ABS, Op_ASL, Op_ORA>(c); c.last_cycles = 6; };
    core.instr_table[0x1F] = +[](Core& c){ ExecRMW_ALU<ABSX, Op_ASL, Op_ORA>(c); c.last_cycles = 7; };
    core.instr_table[0x2F] = +[](Core& c){ ExecRMW_ALU<ABS, Op_ROL, Op_AND>(c); c.last_cycles = 6; };
    core.instr_table[0x3F] = +[](Core& c){ ExecRMW_ALU<ABSX, Op_ROL, Op_AND>(c); c.last_cycles = 7; };
    core.instr_table[0x4F] = +[](Core& c){ ExecRMW_ALU<ABS, Op_LSR, Op_EOR>(c); c.last_cycles = 6; };
    core.instr_table[0x5F] = +[](Core& c){ ExecRMW_ALU<ABSX, Op_LSR, Op_EOR>(c); c.last_cycles = 7; };
    core.instr_table[0x6F] = +[](Core& c){ ExecRMW_ALU<ABS, Op_ROR, Op_ADC>(c); c.last_cycles = 6; };
    core.instr_table[0x7F] = +[](Core& c){ ExecRMW_ALU<ABSX, Op_ROR, Op_ADC>(c); c.last_cycles = 7; };
    // SAX Abs: 4
    core.instr_table[0x8F] = +[](Core& c){ ExecWrite<ABS, Src_AX>(c); c.last_cycles = 4; };
    // SHA Abs,Y: 5
    core.instr_table[0x9F] = +[](Core& c){ ExecSHA(c); c.last_cycles = 5; };
    // LAX Abs: 4. Abs,Y: 4
    core.instr_table[0xAF] = +[](Core& c){ ExecRead<ABS, Op_LAX>(c); c.last_cycles = 4; };
    core.instr_table[0xBF] = +[](Core& c){ ExecRead<ABSY, Op_LAX>(c); c.last_cycles = 4; };
    // DCP/ISB Abs: 6. Abs,X: 7.
    core.instr_table[0xCF] = +[](Core& c){ ExecRMW_ALU<ABS, Op_DEC, Op_CMP>(c); c.last_cycles = 6; };
    core.instr_table[0xDF] = +[](Core& c){ ExecRMW_ALU<ABSX, Op_DEC, Op_CMP>(c); c.last_cycles = 7; };
    core.instr_table[0xEF] = +[](Core& c){ ExecRMW_ALU<ABS, Op_INC, Op_SBC>(c); c.last_cycles = 6; };
    core.instr_table[0xFF] = +[](Core& c){ ExecRMW_ALU<ABSX, Op_INC, Op_SBC>(c); c.last_cycles = 7; };
}