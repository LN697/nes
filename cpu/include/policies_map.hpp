#pragma once

#include "core_policies.hpp"

static void init_instr_table(Core &core) {
    // Fill with nullptr first
    core.instr_table.fill(nullptr);
    
    // Column 0
    core.instr_table[0x00] = +[](Core& core){ Op_BRK::exec(core, 0); };
    core.instr_table[0x10] = +[](Core& core){ ExecBranch<Cond_BPL>(core); };
    core.instr_table[0x20] = +[](Core& core){ ExecJSR(core); };
    core.instr_table[0x30] = +[](Core& core){ ExecBranch<Cond_BMI>(core); };
    core.instr_table[0x40] = +[](Core& core){ Op_RTI::exec(core, 0); };
    core.instr_table[0x50] = +[](Core& core){ ExecBranch<Cond_BVC>(core); };
    core.instr_table[0x60] = +[](Core& core){ Op_RTS::exec(core, 0); };
    core.instr_table[0x70] = +[](Core& core){ ExecBranch<Cond_BVS>(core); };
    core.instr_table[0x80] = +[](Core& core){ ExecRead<IMM, Op_NOP>(core); };
    core.instr_table[0x90] = +[](Core& core){ ExecBranch<Cond_BCC>(core); };
    core.instr_table[0xa0] = +[](Core& core){ ExecRead<IMM, Op_LDY>(core); };
    core.instr_table[0xb0] = +[](Core& core){ ExecBranch<Cond_BCS>(core); };
    core.instr_table[0xC0] = +[](Core& core){ ExecRead<IMM, Op_CPY>(core); };
    core.instr_table[0xD0] = +[](Core& core){ ExecBranch<Cond_BNE>(core); };
    core.instr_table[0xE0] = +[](Core& core){ ExecRead<IMM, Op_CPX>(core); };
    core.instr_table[0xF0] = +[](Core& core){ ExecBranch<Cond_BEQ>(core); };

    // Column 1
    core.instr_table[0x01] = +[](Core& core){ ExecRead<INDX, Op_ORA>(core); };
    core.instr_table[0x11] = +[](Core& core){ ExecRead<INDY, Op_ORA>(core); };
    core.instr_table[0x21] = +[](Core& core){ ExecRead<INDX, Op_AND>(core); };
    core.instr_table[0x31] = +[](Core& core){ ExecRead<INDY, Op_AND>(core); };
    core.instr_table[0x41] = +[](Core& core){ ExecRead<INDX, Op_EOR>(core); };
    core.instr_table[0x51] = +[](Core& core){ ExecRead<INDY, Op_EOR>(core); };
    core.instr_table[0x61] = +[](Core& core){ ExecRead<INDX, Op_ADC>(core); };
    core.instr_table[0x71] = +[](Core& core){ ExecRead<INDY, Op_ADC>(core); };
    core.instr_table[0x81] = +[](Core& core){ ExecWrite<INDX, Src_A>(core); };
    core.instr_table[0x91] = +[](Core& core){ ExecWrite<INDY, Src_A>(core); };
    core.instr_table[0xa1] = +[](Core& core){ ExecRead<INDX, Op_LDA>(core); };
    core.instr_table[0xb1] = +[](Core& core){ ExecRead<INDY, Op_LDA>(core); };
    core.instr_table[0xc1] = +[](Core& core){ ExecRead<INDX, Op_CMP>(core); };
    core.instr_table[0xd1] = +[](Core& core){ ExecRead<INDY, Op_CMP>(core); };
    core.instr_table[0xe1] = +[](Core& core){ ExecRead<INDX, Op_SBC>(core); };
    core.instr_table[0xf1] = +[](Core& core){ ExecRead<INDY, Op_SBC>(core); };

    // Column 2
    core.instr_table[0x02] = +[](Core& core){ Op_JAM::exec(core, 0); };
    core.instr_table[0x12] = +[](Core& core){ Op_JAM::exec(core, 0); };
    core.instr_table[0x22] = +[](Core& core){ Op_JAM::exec(core, 0); };
    core.instr_table[0x32] = +[](Core& core){ Op_JAM::exec(core, 0); };
    core.instr_table[0x42] = +[](Core& core){ Op_JAM::exec(core, 0); };
    core.instr_table[0x52] = +[](Core& core){ Op_JAM::exec(core, 0); };
    core.instr_table[0x62] = +[](Core& core){ Op_JAM::exec(core, 0); };
    core.instr_table[0x72] = +[](Core& core){ Op_JAM::exec(core, 0); };
    core.instr_table[0x82] = +[](Core& core){ ExecRead<IMM, Op_NOP>(core); };
    core.instr_table[0x92] = +[](Core& core){ Op_JAM::exec(core, 0); };
    core.instr_table[0xa2] = +[](Core& core){ ExecRead<IMM, Op_LDX>(core); };
    core.instr_table[0xb2] = +[](Core& core){ Op_JAM::exec(core, 0); };
    core.instr_table[0xc2] = +[](Core& core){ ExecRead<IMM, Op_NOP>(core); };
    core.instr_table[0xd2] = +[](Core& core){ Op_JAM::exec(core, 0); };
    core.instr_table[0xe2] = +[](Core& core){ ExecRead<IMM, Op_NOP>(core); };
    core.instr_table[0xf2] = +[](Core& core){ Op_JAM::exec(core, 0); };

    // Column 3 (Combos)
    core.instr_table[0x03] = +[](Core& c){ ExecRMW_ALU<INDX, Op_ASL, Op_ORA>(c); };
    core.instr_table[0x13] = +[](Core& c){ ExecRMW_ALU<INDY, Op_ASL, Op_ORA>(c); };
    core.instr_table[0x23] = +[](Core& c){ ExecRMW_ALU<INDX, Op_ROL, Op_AND>(c); };
    core.instr_table[0x33] = +[](Core& c){ ExecRMW_ALU<INDY, Op_ROL, Op_AND>(c); };
    core.instr_table[0x43] = +[](Core& c){ ExecRMW_ALU<INDX, Op_LSR, Op_EOR>(c); };
    core.instr_table[0x53] = +[](Core& c){ ExecRMW_ALU<INDY, Op_LSR, Op_EOR>(c); };
    core.instr_table[0x63] = +[](Core& c){ ExecRMW_ALU<INDX, Op_ROR, Op_ADC>(c); };
    core.instr_table[0x73] = +[](Core& c){ ExecRMW_ALU<INDY, Op_ROR, Op_ADC>(c); };
    core.instr_table[0x83] = +[](Core& c){ ExecWrite<INDX, Src_AX>(c); };
    core.instr_table[0x93] = +[](Core& c){ ExecSHA_INDY(c); };
    core.instr_table[0xA3] = +[](Core& c){ ExecRead<INDX, Op_LAX>(c); };
    core.instr_table[0xB3] = +[](Core& c){ ExecRead<INDY, Op_LAX>(c); };
    core.instr_table[0xC3] = +[](Core& c){ ExecRMW_ALU<INDX, Op_DEC, Op_CMP>(c); };
    core.instr_table[0xD3] = +[](Core& c){ ExecRMW_ALU<INDY, Op_DEC, Op_CMP>(c); };
    core.instr_table[0xE3] = +[](Core& c){ ExecRMW_ALU<INDX, Op_INC, Op_SBC>(c); };
    core.instr_table[0xF3] = +[](Core& c){ ExecRMW_ALU<INDY, Op_INC, Op_SBC>(c); };

    // Column 4 (ZP)
    core.instr_table[0x04] = +[](Core& core){ ExecRead<ZP, Op_NOP>(core); };
    core.instr_table[0x14] = +[](Core& core){ ExecRead<ZPX, Op_NOP>(core); };
    core.instr_table[0x24] = +[](Core& core){ ExecRead<ZP, Op_BIT>(core); };
    core.instr_table[0x34] = +[](Core& core){ ExecRead<ZPX, Op_NOP>(core); };
    core.instr_table[0x44] = +[](Core& core){ ExecRead<ZP, Op_NOP>(core); };
    core.instr_table[0x54] = +[](Core& core){ ExecRead<ZPX, Op_NOP>(core); };
    core.instr_table[0x64] = +[](Core& core){ ExecRead<ZP, Op_NOP>(core); };
    core.instr_table[0x74] = +[](Core& core){ ExecRead<ZPX, Op_NOP>(core); };
    core.instr_table[0x84] = +[](Core& core){ ExecWrite<ZP, Src_Y>(core); };
    core.instr_table[0x94] = +[](Core& core){ ExecWrite<ZPX, Src_Y>(core); };
    core.instr_table[0xA4] = +[](Core& core){ ExecRead<ZP, Op_LDY>(core); };
    core.instr_table[0xB4] = +[](Core& core){ ExecRead<ZPX, Op_LDY>(core); };
    core.instr_table[0xC4] = +[](Core& core){ ExecRead<ZP, Op_CPY>(core); };
    core.instr_table[0xD4] = +[](Core& core){ ExecRead<ZPX, Op_NOP>(core); };
    core.instr_table[0xE4] = +[](Core& core){ ExecRead<ZP, Op_CPX>(core); };
    core.instr_table[0xF4] = +[](Core& core){ ExecRead<ZPX, Op_NOP>(core); };

    // Column 5 (ZP, X)
    core.instr_table[0x05] = +[](Core& core){ ExecRead<ZP, Op_ORA>(core); };
    core.instr_table[0x15] = +[](Core& core){ ExecRead<ZPX, Op_ORA>(core); };
    core.instr_table[0x25] = +[](Core& core){ ExecRead<ZP, Op_AND>(core); };
    core.instr_table[0x35] = +[](Core& core){ ExecRead<ZPX, Op_AND>(core); };
    core.instr_table[0x45] = +[](Core& core){ ExecRead<ZP, Op_EOR>(core); };
    core.instr_table[0x55] = +[](Core& core){ ExecRead<ZPX, Op_EOR>(core); };
    core.instr_table[0x65] = +[](Core& core){ ExecRead<ZP, Op_ADC>(core); };
    core.instr_table[0x75] = +[](Core& core){ ExecRead<ZPX, Op_ADC>(core); };
    core.instr_table[0x85] = +[](Core& core){ ExecWrite<ZP, Src_A>(core); };
    core.instr_table[0x95] = +[](Core& core){ ExecWrite<ZPX, Src_A>(core); };
    core.instr_table[0xA5] = +[](Core& core){ ExecRead<ZP, Op_LDA>(core); };
    core.instr_table[0xB5] = +[](Core& core){ ExecRead<ZPX, Op_LDA>(core); };
    core.instr_table[0xC5] = +[](Core& core){ ExecRead<ZP, Op_CMP>(core); };
    core.instr_table[0xD5] = +[](Core& core){ ExecRead<ZPX, Op_CMP>(core); };
    core.instr_table[0xE5] = +[](Core& core){ ExecRead<ZP, Op_SBC>(core); };
    core.instr_table[0xF5] = +[](Core& core){ ExecRead<ZPX, Op_SBC>(core); };

    // Column 6 (ZP RMW)
    core.instr_table[0x06] = +[](Core& core){ ExecRMW<ZP, Op_ASL>(core); };
    core.instr_table[0x16] = +[](Core& core){ ExecRMW<ZPX, Op_ASL>(core); };
    core.instr_table[0x26] = +[](Core& core){ ExecRMW<ZP, Op_ROL>(core); };
    core.instr_table[0x36] = +[](Core& core){ ExecRMW<ZPX, Op_ROL>(core); };
    core.instr_table[0x46] = +[](Core& core){ ExecRMW<ZP, Op_LSR>(core); };
    core.instr_table[0x56] = +[](Core& core){ ExecRMW<ZPX, Op_LSR>(core); };
    core.instr_table[0x66] = +[](Core& core){ ExecRMW<ZP, Op_ROR>(core); };
    core.instr_table[0x76] = +[](Core& core){ ExecRMW<ZPX, Op_ROR>(core); };
    core.instr_table[0x86] = +[](Core& core){ ExecWrite<ZP, Src_X>(core); };
    core.instr_table[0x96] = +[](Core& core){ ExecWrite<ZPY, Src_X>(core); };
    core.instr_table[0xA6] = +[](Core& core){ ExecRead<ZP, Op_LDX>(core); };
    core.instr_table[0xB6] = +[](Core& core){ ExecRead<ZPY, Op_LDX>(core); };
    core.instr_table[0xC6] = +[](Core& core){ ExecRMW<ZP, Op_DEC>(core); };
    core.instr_table[0xD6] = +[](Core& core){ ExecRMW<ZPX, Op_DEC>(core); };
    core.instr_table[0xE6] = +[](Core& core){ ExecRMW<ZP, Op_INC>(core); };
    core.instr_table[0xF6] = +[](Core& core){ ExecRMW<ZPX, Op_INC>(core); };

    // Column 7 (Combos)
    core.instr_table[0x07] = +[](Core& c){ ExecRMW_ALU<ZP, Op_ASL, Op_ORA>(c); };
    core.instr_table[0x17] = +[](Core& c){ ExecRMW_ALU<ZPX, Op_ASL, Op_ORA>(c); };
    core.instr_table[0x27] = +[](Core& c){ ExecRMW_ALU<ZP, Op_ROL, Op_AND>(c); };
    core.instr_table[0x37] = +[](Core& c){ ExecRMW_ALU<ZPX, Op_ROL, Op_AND>(c); };
    core.instr_table[0x47] = +[](Core& c){ ExecRMW_ALU<ZP, Op_LSR, Op_EOR>(c); };
    core.instr_table[0x57] = +[](Core& c){ ExecRMW_ALU<ZPX, Op_LSR, Op_EOR>(c); };
    core.instr_table[0x67] = +[](Core& c){ ExecRMW_ALU<ZP, Op_ROR, Op_ADC>(c); };
    core.instr_table[0x77] = +[](Core& c){ ExecRMW_ALU<ZPX, Op_ROR, Op_ADC>(c); };
    core.instr_table[0x87] = +[](Core& c){ ExecWrite<ZP, Src_AX>(c); };
    core.instr_table[0x97] = +[](Core& c){ ExecWrite<ZPY, Src_AX>(c); };
    core.instr_table[0xA7] = +[](Core& c){ ExecRead<ZP, Op_LAX>(c); };
    core.instr_table[0xB7] = +[](Core& c){ ExecRead<ZPY, Op_LAX>(c); };
    core.instr_table[0xC7] = +[](Core& c){ ExecRMW_ALU<ZP, Op_DEC, Op_CMP>(c); };
    core.instr_table[0xD7] = +[](Core& c){ ExecRMW_ALU<ZPX, Op_DEC, Op_CMP>(c); };
    core.instr_table[0xE7] = +[](Core& c){ ExecRMW_ALU<ZP, Op_INC, Op_SBC>(c); };
    core.instr_table[0xF7] = +[](Core& c){ ExecRMW_ALU<ZPX, Op_INC, Op_SBC>(c); };

    // Column 8 (Implied)
    core.instr_table[0x08] = +[](Core& core){ Op_PHP::exec(core, 0); };
    core.instr_table[0x18] = +[](Core& core){ Op_CLC::exec(core, 0); };
    core.instr_table[0x28] = +[](Core& core){ Op_PLP::exec(core, 0); };
    core.instr_table[0x38] = +[](Core& core){ Op_SEC::exec(core, 0); };
    core.instr_table[0x48] = +[](Core& core){ Op_PHA::exec(core, 0); };
    core.instr_table[0x58] = +[](Core& core){ Op_CLI::exec(core, 0); };
    core.instr_table[0x68] = +[](Core& core){ Op_PLA::exec(core, 0); };
    core.instr_table[0x78] = +[](Core& core){ Op_SEI::exec(core, 0); };
    core.instr_table[0x88] = +[](Core& core){ Op_DEY::exec(core, 0); };
    core.instr_table[0x98] = +[](Core& core){ Op_TYA::exec(core, 0); };
    core.instr_table[0xA8] = +[](Core& core){ Op_TAY::exec(core, 0); };
    core.instr_table[0xB8] = +[](Core& core){ Op_CLV::exec(core, 0); };
    core.instr_table[0xC8] = +[](Core& core){ Op_INY::exec(core, 0); };
    core.instr_table[0xD8] = +[](Core& core){ Op_CLD::exec(core, 0); };
    core.instr_table[0xE8] = +[](Core& core){ Op_INX::exec(core, 0); };
    core.instr_table[0xF8] = +[](Core& core){ Op_SED::exec(core, 0); };

    // Column 9 (Abs Y)
    core.instr_table[0x09] = +[](Core& core){ ExecRead<IMM, Op_ORA>(core); };
    core.instr_table[0x19] = +[](Core& core){ ExecRead<ABSY, Op_ORA>(core); };
    core.instr_table[0x29] = +[](Core& core){ ExecRead<IMM, Op_AND>(core); };
    core.instr_table[0x39] = +[](Core& core){ ExecRead<ABSY, Op_AND>(core); };
    core.instr_table[0x49] = +[](Core& core){ ExecRead<IMM, Op_EOR>(core); };
    core.instr_table[0x59] = +[](Core& core){ ExecRead<ABSY, Op_EOR>(core); };
    core.instr_table[0x69] = +[](Core& core){ ExecRead<IMM, Op_ADC>(core); };
    core.instr_table[0x79] = +[](Core& core){ ExecRead<ABSY, Op_ADC>(core); };
    core.instr_table[0x89] = +[](Core& core){ ExecRead<IMM, Op_NOP>(core); };
    core.instr_table[0x99] = +[](Core& core){ ExecWrite<ABSY, Src_A>(core); };
    core.instr_table[0xA9] = +[](Core& core){ ExecRead<IMM, Op_LDA>(core); };
    core.instr_table[0xB9] = +[](Core& core){ ExecRead<ABSY, Op_LDA>(core); };
    core.instr_table[0xC9] = +[](Core& core){ ExecRead<IMM, Op_CMP>(core); };
    core.instr_table[0xD9] = +[](Core& core){ ExecRead<ABSY, Op_CMP>(core); };
    core.instr_table[0xE9] = +[](Core& core){ ExecRead<IMM, Op_SBC>(core); };
    core.instr_table[0xF9] = +[](Core& core){ ExecRead<ABSY, Op_SBC>(core); };

    // Column A (Accumulator / Transfers)
    core.instr_table[0x0A] = +[](Core& core){ ExecRMW<ACC, Op_ASL>(core); };
    core.instr_table[0x1A] = +[](Core& core){ Op_NOP::exec(core, 0); };
    core.instr_table[0x2A] = +[](Core& core){ ExecRMW<ACC, Op_ROL>(core); };
    core.instr_table[0x3A] = +[](Core& core){ Op_NOP::exec(core, 0); };
    core.instr_table[0x4A] = +[](Core& core){ ExecRMW<ACC, Op_LSR>(core); };
    core.instr_table[0x5A] = +[](Core& core){ Op_NOP::exec(core, 0); };
    core.instr_table[0x6A] = +[](Core& core){ ExecRMW<ACC, Op_ROR>(core); };
    core.instr_table[0x7A] = +[](Core& core){ Op_NOP::exec(core, 0); };
    core.instr_table[0x8A] = +[](Core& core){ Op_TXA::exec(core, 0); };
    core.instr_table[0x9A] = +[](Core& core){ Op_TXS::exec(core, 0); };
    core.instr_table[0xAA] = +[](Core& core){ Op_TAX::exec(core, 0); };
    core.instr_table[0xBA] = +[](Core& core){ Op_TSX::exec(core, 0); };
    core.instr_table[0xCA] = +[](Core& core){ Op_DEX::exec(core, 0); };
    core.instr_table[0xDA] = +[](Core& core){ Op_NOP::exec(core, 0); };
    core.instr_table[0xEA] = +[](Core& core){ Op_NOP::exec(core, 0); };
    core.instr_table[0xFA] = +[](Core& core){ Op_NOP::exec(core, 0); };

    // Column B (Immediate Illegal / Abs Y Combos)
    core.instr_table[0x0B] = +[](Core& c){ ExecRead<IMM, Op_ANC>(c); };
    core.instr_table[0x1B] = +[](Core& c){ ExecRMW_ALU<ABSY, Op_ASL, Op_ORA>(c); };
    core.instr_table[0x2B] = +[](Core& c){ ExecRead<IMM, Op_ANC>(c); };
    core.instr_table[0x3B] = +[](Core& c){ ExecRMW_ALU<ABSY, Op_ROL, Op_AND>(c); };
    core.instr_table[0x4B] = +[](Core& c){ ExecRead<IMM, Op_ALR>(c); };
    core.instr_table[0x5B] = +[](Core& c){ ExecRMW_ALU<ABSY, Op_LSR, Op_EOR>(c); };
    core.instr_table[0x6B] = +[](Core& c){ ExecRead<IMM, Op_ARR>(c); };
    core.instr_table[0x7B] = +[](Core& c){ ExecRMW_ALU<ABSY, Op_ROR, Op_ADC>(c); };
    core.instr_table[0x8B] = +[](Core& c){ ExecRead<IMM, Op_XAA>(c); };
    core.instr_table[0x9B] = +[](Core& c){ ExecTAS(c); };
    core.instr_table[0xAB] = +[](Core& c){ ExecRead<IMM, Op_ATX>(c); };
    core.instr_table[0xBB] = +[](Core& c){ ExecRead<ABSY, Op_LAS>(c); };
    core.instr_table[0xCB] = +[](Core& c){ ExecRead<IMM, Op_AXS>(c); };
    core.instr_table[0xDB] = +[](Core& c){ ExecRMW_ALU<ABSY, Op_DEC, Op_CMP>(c); };
    core.instr_table[0xEB] = +[](Core& c){ ExecRead<IMM, Op_SBC>(c); };
    core.instr_table[0xFB] = +[](Core& c){ ExecRMW_ALU<ABSY, Op_INC, Op_SBC>(c); };

    // Column C (Abs)
    core.instr_table[0x0C] = +[](Core& core){ ExecRead<ABS, Op_NOP>(core); };
    core.instr_table[0x1C] = +[](Core& core){ ExecRead<ABSX, Op_NOP>(core); };
    core.instr_table[0x2C] = +[](Core& core){ ExecRead<ABS, Op_BIT>(core); };
    core.instr_table[0x3C] = +[](Core& core){ ExecRead<ABSX, Op_NOP>(core); };
    core.instr_table[0x4C] = +[](Core& core){ ExecJMP<ABS>(core); };
    core.instr_table[0x5C] = +[](Core& core){ ExecRead<ABSX, Op_NOP>(core); };
    core.instr_table[0x6C] = +[](Core& core){ ExecJMP<IND>(core); };
    core.instr_table[0x7C] = +[](Core& core){ ExecRead<ABSX, Op_NOP>(core); };
    core.instr_table[0x8C] = +[](Core& core){ ExecWrite<ABS, Src_Y>(core); };
    core.instr_table[0x9C] = +[](Core& core){ ExecSHY(core); };
    core.instr_table[0xAC] = +[](Core& core){ ExecRead<ABS, Op_LDY>(core); };
    core.instr_table[0xBC] = +[](Core& core){ ExecRead<ABSX, Op_LDY>(core); };
    core.instr_table[0xCC] = +[](Core& core){ ExecRead<ABS, Op_CPY>(core); };
    core.instr_table[0xDC] = +[](Core& core){ ExecRead<ABSX, Op_NOP>(core); };
    core.instr_table[0xEC] = +[](Core& core){ ExecRead<ABS, Op_CPX>(core); };
    core.instr_table[0xFC] = +[](Core& core){ ExecRead<ABSX, Op_NOP>(core); };

    // Column D (Abs X ALU)
    core.instr_table[0x0D] = +[](Core& core){ ExecRead<ABS, Op_ORA>(core); };
    core.instr_table[0x1D] = +[](Core& core){ ExecRead<ABSX, Op_ORA>(core); };
    core.instr_table[0x2D] = +[](Core& core){ ExecRead<ABS, Op_AND>(core); };
    core.instr_table[0x3D] = +[](Core& core){ ExecRead<ABSX, Op_AND>(core); };
    core.instr_table[0x4D] = +[](Core& core){ ExecRead<ABS, Op_EOR>(core); };
    core.instr_table[0x5D] = +[](Core& core){ ExecRead<ABSX, Op_EOR>(core); };
    core.instr_table[0x6D] = +[](Core& core){ ExecRead<ABS, Op_ADC>(core); };
    core.instr_table[0x7D] = +[](Core& core){ ExecRead<ABSX, Op_ADC>(core); };
    core.instr_table[0x8D] = +[](Core& core){ ExecWrite<ABS, Src_A>(core); };
    core.instr_table[0x9D] = +[](Core& core){ ExecWrite<ABSX, Src_A>(core); };
    core.instr_table[0xAD] = +[](Core& core){ ExecRead<ABS, Op_LDA>(core); };
    core.instr_table[0xBD] = +[](Core& core){ ExecRead<ABSX, Op_LDA>(core); };
    core.instr_table[0xCD] = +[](Core& core){ ExecRead<ABS, Op_CMP>(core); };
    core.instr_table[0xDD] = +[](Core& core){ ExecRead<ABSX, Op_CMP>(core); };
    core.instr_table[0xED] = +[](Core& core){ ExecRead<ABS, Op_SBC>(core); };
    core.instr_table[0xFD] = +[](Core& core){ ExecRead<ABSX, Op_SBC>(core); };

    // Column E (Abs RMW)
    core.instr_table[0x0E] = +[](Core& core){ ExecRMW<ABS, Op_ASL>(core); };
    core.instr_table[0x1E] = +[](Core& core){ ExecRMW<ABSX, Op_ASL>(core); };
    core.instr_table[0x2E] = +[](Core& core){ ExecRMW<ABS, Op_ROL>(core); };
    core.instr_table[0x3E] = +[](Core& core){ ExecRMW<ABSX, Op_ROL>(core); };
    core.instr_table[0x4E] = +[](Core& core){ ExecRMW<ABS, Op_LSR>(core); };
    core.instr_table[0x5E] = +[](Core& core){ ExecRMW<ABSX, Op_LSR>(core); };
    core.instr_table[0x6E] = +[](Core& core){ ExecRMW<ABS, Op_ROR>(core); };
    core.instr_table[0x7E] = +[](Core& core){ ExecRMW<ABSX, Op_ROR>(core); };
    core.instr_table[0x8E] = +[](Core& core){ ExecWrite<ABS, Src_X>(core); };
    core.instr_table[0x9E] = +[](Core& c){ ExecSHX(c); };
    core.instr_table[0xAE] = +[](Core& core){ ExecRead<ABS, Op_LDX>(core); };
    core.instr_table[0xBE] = +[](Core& core){ ExecRead<ABSY, Op_LDX>(core); };
    core.instr_table[0xCE] = +[](Core& core){ ExecRMW<ABS, Op_DEC>(core); };
    core.instr_table[0xDE] = +[](Core& core){ ExecRMW<ABSX, Op_DEC>(core); };
    core.instr_table[0xEE] = +[](Core& core){ ExecRMW<ABS, Op_INC>(core); };
    core.instr_table[0xFE] = +[](Core& core){ ExecRMW<ABSX, Op_INC>(core); };

    // Column F (Abs Combos)
    core.instr_table[0x0F] = +[](Core& c){ ExecRMW_ALU<ABS, Op_ASL, Op_ORA>(c); };
    core.instr_table[0x1F] = +[](Core& c){ ExecRMW_ALU<ABSX, Op_ASL, Op_ORA>(c); };
    core.instr_table[0x2F] = +[](Core& c){ ExecRMW_ALU<ABS, Op_ROL, Op_AND>(c); };
    core.instr_table[0x3F] = +[](Core& c){ ExecRMW_ALU<ABSX, Op_ROL, Op_AND>(c); };
    core.instr_table[0x4F] = +[](Core& c){ ExecRMW_ALU<ABS, Op_LSR, Op_EOR>(c); };
    core.instr_table[0x5F] = +[](Core& c){ ExecRMW_ALU<ABSX, Op_LSR, Op_EOR>(c); };
    core.instr_table[0x6F] = +[](Core& c){ ExecRMW_ALU<ABS, Op_ROR, Op_ADC>(c); };
    core.instr_table[0x7F] = +[](Core& c){ ExecRMW_ALU<ABSX, Op_ROR, Op_ADC>(c); };
    core.instr_table[0x8F] = +[](Core& c){ ExecWrite<ABS, Src_AX>(c); };
    core.instr_table[0x9F] = +[](Core& c){ ExecSHA(c); };
    core.instr_table[0xAF] = +[](Core& c){ ExecRead<ABS, Op_LAX>(c); };
    core.instr_table[0xBF] = +[](Core& c){ ExecRead<ABSY, Op_LAX>(c); };
    core.instr_table[0xCF] = +[](Core& c){ ExecRMW_ALU<ABS, Op_DEC, Op_CMP>(c); };
    core.instr_table[0xDF] = +[](Core& c){ ExecRMW_ALU<ABSX, Op_DEC, Op_CMP>(c); };
    core.instr_table[0xEF] = +[](Core& c){ ExecRMW_ALU<ABS, Op_INC, Op_SBC>(c); };
    core.instr_table[0xFF] = +[](Core& c){ ExecRMW_ALU<ABSX, Op_INC, Op_SBC>(c); };
}