#pragma once

#include <cstdint>
#include <type_traits>
#include "core.hpp"

// =============================================================
// ADDRESSING MODES
// =============================================================

struct IMP {
    static constexpr bool is_implied = true;
    static uint16_t getAddr(Core& core) { (void)core; return 0; } 
};

struct IMM {
    static constexpr bool is_implied = false;
    static std::uint16_t getAddr(Core& core) {
        std::uint16_t address = static_cast<std::uint16_t>(core.pc.getValue());
        core.pc.setValue(static_cast<std::uint64_t>(address + 1));
        return address;
    }
};

struct ZP {
    static constexpr bool is_implied = false;
    static std::uint16_t getAddr(Core& core) { return core.fetch(); }
};

struct ZPX {
    static constexpr bool is_implied = false;
    static uint16_t getAddr(Core& core) { 
        return static_cast<uint16_t>((core.fetch() + static_cast<uint8_t>(core.x.getValue())) & 0xFF);
    }
};

struct ZPY {
    static constexpr bool is_implied = false;
    static uint16_t getAddr(Core& core) { 
        return static_cast<uint16_t>((core.fetch() + static_cast<uint8_t>(core.y.getValue())) & 0xFF);
    }
};

struct ABS {
    static constexpr bool is_implied = false;
    static uint16_t getAddr(Core& core) { return core.fetchWord(); }
};

struct ABSX {
    static constexpr bool is_implied = false;
    static uint16_t getAddr(Core& core) { return static_cast<uint16_t>(core.fetchWord() + static_cast<uint16_t>(core.x.getValue())); }
};

struct ABSY {
    static constexpr bool is_implied = false;
    static uint16_t getAddr(Core& core) { return static_cast<uint16_t>(core.fetchWord() + static_cast<uint16_t>(core.y.getValue())); }
};

struct IND {
    static constexpr bool is_implied = false;
    static uint16_t getAddr(Core& core) {
        uint16_t ptr = core.fetchWord(); // e.g., 0xB7FF
        
        // 1. Read Low Byte from the pointer address
        uint16_t low = core.read(ptr);
        
        // 2. Hardware Bug: The High Byte is read from the same page.
        // We do this by taking the Page of ptr (ptr & 0xFF00)
        // and adding the wrap-around low byte ((ptr + 1) & 0xFF).
        uint16_t highAddr = (ptr & 0xFF00) | ((ptr + 1) & 0xFF);
        
        // In the failing case:
        // ptr = 0xB7FF
        // Page = 0xB700
        // (ptr+1)&0xFF = 0x00
        // highAddr = 0xB700 (Correct!)
        
        uint16_t high = core.read(highAddr);
        
        return (high << 8) | low;
    }
};

struct INDX {
    static constexpr bool is_implied = false;
    static uint16_t getAddr(Core& core) {
        uint8_t ptr = static_cast<uint8_t>((core.fetch() + static_cast<uint8_t>(core.x.getValue())) & 0xFF); 
        uint16_t low = core.read(ptr);
        uint16_t high = core.read((ptr + 1) & 0xFF);
        return (high << 8) | low;
    }
};

struct INDY {
    static constexpr bool is_implied = false;
    static uint16_t getAddr(Core& core) {
        uint8_t ptr = core.fetch();
        uint16_t low = core.read(ptr);
        uint16_t high = core.read((ptr + 1) & 0xFF);
        return static_cast<uint16_t>(((high << 8) | low) + static_cast<uint16_t>(core.y.getValue()));
    }
};

struct REL {
    static constexpr bool is_implied = false;
    static uint16_t getAddr(Core& core) {
        int8_t offset = static_cast<int8_t>(core.fetch()); 
        return static_cast<uint16_t>(core.pc.getValue() + offset);
    }
};

struct ACC {
    static constexpr bool is_implied = true; 
    static uint16_t getAddr(Core& core) { (void)core; return 0; }
};

// =============================================================
// EXECUTION TEMPLATES
// =============================================================

template <typename MODE, typename OPERATION>
void ExecRead(Core& core) {
    uint16_t addr = MODE::getAddr(core);
    uint8_t  val  = core.read(addr);
    OPERATION::exec(core, val);
}

template <typename MODE, typename SOURCE>
void ExecWrite(Core& core) {
    uint16_t addr = MODE::getAddr(core);
    uint8_t  val  = SOURCE::get(core);
    core.write(addr, val);
}

template <typename MODE, typename OPERATION>
void ExecRMW(Core& core) {
    if constexpr (std::is_same_v<MODE, ACC>) {
        uint8_t result = OPERATION::calc(core, static_cast<uint8_t>(core.a.getValue()));
        core.a.setValue(result);
    } else {
        uint16_t addr = MODE::getAddr(core);
        uint8_t  val  = core.read(addr);
        uint8_t  res  = OPERATION::calc(core, val);
        core.write(addr, res);
    }
}

template <typename CONDITION>
void ExecBranch(Core& core) {
    // REL mode returns the TARGET address, not the offset, based on your implementation
    uint16_t target = REL::getAddr(core); 
    if (CONDITION::check(core)) {
        core.pc.setValue(target);
        // Note: Real hardware adds +1 cycle here, and +2 if page crossed
    }
}

// =============================================================
// OPERATIONS: LOAD / STORE
// =============================================================

struct Op_LDA { 
    static void exec(Core& core, uint8_t v) { 
        core.a.setValue(v);
        core.setStatusFlag(Core::StatusFlag::Z, v == 0);
        core.setStatusFlag(Core::StatusFlag::N, (v & 0x80) != 0);
    }
};

struct Op_LDX { 
    static void exec(Core& core, uint8_t v) { 
        core.x.setValue(v); 
        core.setStatusFlag(Core::StatusFlag::Z, v == 0);
        core.setStatusFlag(Core::StatusFlag::N, (v & 0x80) != 0);
    } 
};

struct Op_LDY { 
    static void exec(Core& core, uint8_t v) { 
        core.y.setValue(v); 
        core.setStatusFlag(Core::StatusFlag::Z, v == 0);
        core.setStatusFlag(Core::StatusFlag::N, (v & 0x80) != 0);
    } 
};

struct Src_A { static uint8_t get(Core& core) { return static_cast<uint8_t>(core.a.getValue()); } };
struct Src_X { static uint8_t get(Core& core) { return static_cast<uint8_t>(core.x.getValue()); } };
struct Src_Y { static uint8_t get(Core& core) { return static_cast<uint8_t>(core.y.getValue()); } };

// =============================================================
// OPERATIONS: LOGICAL
// =============================================================

struct Op_AND { 
    static void exec(Core& core, uint8_t v) { 
        auto newv = core.a.getValue() & v;
        core.a.setValue(newv);
        core.setStatusFlag(Core::StatusFlag::Z, static_cast<uint8_t>(newv) == 0);
        core.setStatusFlag(Core::StatusFlag::N, (static_cast<uint8_t>(newv) & 0x80) != 0);
    } 
};

struct Op_ORA { 
    static void exec(Core& core, uint8_t v) { 
        auto newv = core.a.getValue() | v;
        core.a.setValue(newv);
        core.setStatusFlag(Core::StatusFlag::Z, static_cast<uint8_t>(newv) == 0);
        core.setStatusFlag(Core::StatusFlag::N, (static_cast<uint8_t>(newv) & 0x80) != 0);
    } 
};

struct Op_EOR { 
    static void exec(Core& core, uint8_t v) { 
        auto newv = core.a.getValue() ^ v;
        core.a.setValue(newv);
        core.setStatusFlag(Core::StatusFlag::Z, static_cast<uint8_t>(newv) == 0);
        core.setStatusFlag(Core::StatusFlag::N, (static_cast<uint8_t>(newv) & 0x80) != 0);
    } 
};

struct Op_BIT {
    static void exec(Core& core, uint8_t v) {
        uint8_t a = static_cast<uint8_t>(core.a.getValue());
        uint8_t res = a & v;
        core.setStatusFlag(Core::StatusFlag::Z, res == 0);
        core.setStatusFlag(Core::StatusFlag::N, (v & 0x80) != 0); // Bit 7 of Memory
        core.setStatusFlag(Core::StatusFlag::V, (v & 0x40) != 0); // Bit 6 of Memory
    }
};

// =============================================================
// OPERATIONS: MATH
// =============================================================

struct Op_ADC { 
    static void exec(Core& core, uint8_t v) { 
        uint8_t a = static_cast<uint8_t>(core.a.getValue());
        uint8_t carry = core.getStatusFlag(Core::StatusFlag::C) ? 1 : 0;
        
        // 1. Calculate Binary Sum
        uint16_t binarySum = a + v + carry;

        if (core.getStatusFlag(Core::StatusFlag::D)) {
            // --- DECIMAL MODE LOGIC (NMOS 6502) ---
            
            // 2. Z Flag: Derived directly from the BINARY sum
            // This is a known hardware quirk. Z ignores the BCD fixups.
            core.setStatusFlag(Core::StatusFlag::Z, (binarySum & 0xFF) == 0);

            // 3. Prepare Intermediate Sum for N & V Flags
            uint8_t al = (a & 0x0F) + (v & 0x0F) + carry;
            uint8_t diff = (al > 9) ? 0x06 : 0;
            
            uint16_t nCalc = binarySum + diff;
            
            // "Double Carry Suppression" (Test 64/789 Fix)
            // If both Binary and Correction generated half-carries, suppress the 2nd one.
            if ((al > 15) && (((binarySum & 0x0F) + diff) > 15)) {
                nCalc -= 0x10;
            }
            
            // 4. N Flag: Derived from Intermediate Sum
            core.setStatusFlag(Core::StatusFlag::N, (nCalc & 0x80) != 0);

            // 5. V Flag: Derived from Intermediate Sum (Test 703 Fix)
            // We check if (A_sign equals V_sign) AND (A_sign does NOT equal Result_sign)
            // Result here is the Intermediate Sum (nCalc) masked to 8 bits.
            uint8_t intermediate8 = static_cast<uint8_t>(nCalc);
            bool overflow = (~(a ^ v) & (a ^ intermediate8) & 0x80) != 0;
            core.setStatusFlag(Core::StatusFlag::V, overflow);

            // 6. Final BCD Fixup (Calculates Final Value and C Flag)
            // Re-calculate clean nibbles for storage
            al = (a & 0x0F) + (v & 0x0F) + carry;
            uint8_t ah = (a >> 4) + (v >> 4);
            
            if (al > 9) {
                al += 6;
                ah++; 
            }
            
            if (ah > 9) {
                ah += 6;
            }
            
            // C Flag
            core.setStatusFlag(Core::StatusFlag::C, ah > 0x0F);
            
            // Store Result
            core.a.setValue((ah << 4) | (al & 0x0F));

        } else {
            // --- BINARY MODE LOGIC ---
            core.setStatusFlag(Core::StatusFlag::C, binarySum > 0xFF);
            uint8_t result8 = static_cast<uint8_t>(binarySum);
            bool overflow = (~(a ^ v) & (a ^ result8) & 0x80) != 0;
            core.setStatusFlag(Core::StatusFlag::V, overflow);
            
            core.a.setValue(result8);
            core.setStatusFlag(Core::StatusFlag::Z, result8 == 0);
            core.setStatusFlag(Core::StatusFlag::N, (result8 & 0x80) != 0);
        }
    } 
};

struct Op_SBC {
    static void exec(Core& core, uint8_t v) {
        // Fetch A and Carry
        uint8_t a = static_cast<uint8_t>(core.a.getValue());
        uint8_t c = core.getStatusFlag(Core::StatusFlag::C) ? 1 : 0;
        
        // 1. Binary Subtraction (Used for Flags and result foundation)
        // Formula: A - M - (1 - C)
        uint16_t diff = a - v - (1 - c);

        // 2. Set Flags (Same for Binary and Decimal on NMOS 6502)
        // C Flag: In 6502, C=1 means NO borrow (Result >= 0), C=0 means Borrow (Result < 0)
        core.setStatusFlag(Core::StatusFlag::C, diff <= 0xFF);
        
        // V Flag: Overflow if (Pos - Neg = Neg) or (Neg - Pos = Pos)
        // We check signs of A, ~M (the subtraction operand), and Result
        bool overflow = ((a ^ v) & (a ^ diff) & 0x80) != 0;
        core.setStatusFlag(Core::StatusFlag::V, overflow);
        
        // Z and N are based on the binary result
        core.setStatusFlag(Core::StatusFlag::Z, (diff & 0xFF) == 0);
        core.setStatusFlag(Core::StatusFlag::N, (diff & 0x80) != 0);

        if (core.getStatusFlag(Core::StatusFlag::D)) {
            // --- DECIMAL MODE LOGIC ---
            
            // We must treat nibbles independently to prevent borrow propagation
            // from the low correction corrupting the high nibble.
            
            // Re-calculate nibble differences to detect borrows
            uint16_t lowDiff = (a & 0x0F) - (v & 0x0F) - (1 - c);
            uint16_t highDiff = (a >> 4) - (v >> 4) - ((lowDiff > 0xF) ? 1 : 0); // (lowDiff > 0xF) detects borrow wrap 0x00->0xFF
            
            uint8_t lowNibble = diff & 0x0F;
            uint8_t highNibble = (diff >> 4) & 0x0F;

            // Apply Corrections
            // If the low nibble required a borrow, subtract 6
            if (lowDiff > 0xF) { 
                lowNibble = (lowNibble - 6) & 0x0F;
            }
            
            // If the high nibble required a borrow, subtract 6
            if (highDiff > 0xF) { 
                highNibble = (highNibble - 6) & 0x0F;
            }
            
            // Combine
            core.a.setValue((highNibble << 4) | lowNibble);
            
        } else {
            // --- BINARY MODE LOGIC ---
            core.a.setValue(static_cast<uint8_t>(diff));
        }
    }
};

struct Op_CMP {
    static void exec(Core& core, uint8_t v) {
        uint8_t reg = static_cast<uint8_t>(core.a.getValue());
        uint8_t res = reg - v;
        core.setStatusFlag(Core::StatusFlag::C, reg >= v);
        core.setStatusFlag(Core::StatusFlag::Z, reg == v);
        core.setStatusFlag(Core::StatusFlag::N, (res & 0x80) != 0);
    }
};

struct Op_CPX {
    static void exec(Core& core, uint8_t v) {
        uint8_t reg = static_cast<uint8_t>(core.x.getValue());
        uint8_t res = reg - v;
        core.setStatusFlag(Core::StatusFlag::C, reg >= v);
        core.setStatusFlag(Core::StatusFlag::Z, reg == v);
        core.setStatusFlag(Core::StatusFlag::N, (res & 0x80) != 0);
    }
};

struct Op_CPY {
    static void exec(Core& core, uint8_t v) {
        uint8_t reg = static_cast<uint8_t>(core.y.getValue());
        uint8_t res = reg - v;
        core.setStatusFlag(Core::StatusFlag::C, reg >= v);
        core.setStatusFlag(Core::StatusFlag::Z, reg == v);
        core.setStatusFlag(Core::StatusFlag::N, (res & 0x80) != 0);
    }
};

// =============================================================
// OPERATIONS: READ-MODIFY-WRITE (Shifts, Inc, Dec)
// =============================================================

struct Op_INC { 
    static uint8_t calc(Core& core, uint8_t v) { 
        uint8_t r = v + 1;
        core.setStatusFlag(Core::StatusFlag::Z, r == 0);
        core.setStatusFlag(Core::StatusFlag::N, (r & 0x80) != 0);
        return r; 
    } 
};

struct Op_DEC { 
    static uint8_t calc(Core& core, uint8_t v) { 
        uint8_t r = v - 1;
        core.setStatusFlag(Core::StatusFlag::Z, r == 0);
        core.setStatusFlag(Core::StatusFlag::N, (r & 0x80) != 0);
        return r; 
    } 
};

struct Op_ASL { 
    static uint8_t calc(Core& core, uint8_t v) { 
        core.setStatusFlag(Core::StatusFlag::C, (v & 0x80) != 0); // Carry = Old Bit 7
        uint8_t r = v << 1; 
        core.setStatusFlag(Core::StatusFlag::Z, r == 0);
        core.setStatusFlag(Core::StatusFlag::N, (r & 0x80) != 0);
        return r; 
    } 
};

struct Op_LSR {
    static uint8_t calc(Core& core, uint8_t v) {
        core.setStatusFlag(Core::StatusFlag::C, (v & 0x01) != 0); // Carry = Old Bit 0
        uint8_t r = v >> 1;
        core.setStatusFlag(Core::StatusFlag::Z, r == 0);
        core.setStatusFlag(Core::StatusFlag::N, false); // Bit 7 always 0
        return r;
    }
};

struct Op_ROL {
    static uint8_t calc(Core& core, uint8_t v) {
        bool oldCarry = core.getStatusFlag(Core::StatusFlag::C);
        core.setStatusFlag(Core::StatusFlag::C, (v & 0x80) != 0);
        uint8_t r = (v << 1) | (oldCarry ? 1 : 0);
        core.setStatusFlag(Core::StatusFlag::Z, r == 0);
        core.setStatusFlag(Core::StatusFlag::N, (r & 0x80) != 0);
        return r;
    }
};

struct Op_ROR {
    static uint8_t calc(Core& core, uint8_t v) {
        bool oldCarry = core.getStatusFlag(Core::StatusFlag::C);
        core.setStatusFlag(Core::StatusFlag::C, (v & 0x01) != 0);
        uint8_t r = (v >> 1) | (oldCarry ? 0x80 : 0);
        core.setStatusFlag(Core::StatusFlag::Z, r == 0);
        core.setStatusFlag(Core::StatusFlag::N, (r & 0x80) != 0);
        return r;
    }
};

// =============================================================
// OPERATIONS: BRANCHING
// =============================================================

struct Cond_BPL { static bool check(Core& c) { return !c.getStatusFlag(Core::StatusFlag::N); } };
struct Cond_BMI { static bool check(Core& c) { return c.getStatusFlag(Core::StatusFlag::N); } };
struct Cond_BVC { static bool check(Core& c) { return !c.getStatusFlag(Core::StatusFlag::V); } };
struct Cond_BVS { static bool check(Core& c) { return c.getStatusFlag(Core::StatusFlag::V); } };
struct Cond_BCC { static bool check(Core& c) { return !c.getStatusFlag(Core::StatusFlag::C); } };
struct Cond_BCS { static bool check(Core& c) { return c.getStatusFlag(Core::StatusFlag::C); } };
struct Cond_BNE { static bool check(Core& c) { return !c.getStatusFlag(Core::StatusFlag::Z); } };
struct Cond_BEQ { static bool check(Core& c) { return c.getStatusFlag(Core::StatusFlag::Z); } };

// =============================================================
// OPERATIONS: TRANSFERS & FLAGS (Implied Mode)
// =============================================================

struct Op_TAX { static void exec(Core& c, uint8_t) { 
    uint8_t v = static_cast<uint8_t>(c.a.getValue()); 
    c.x.setValue(v); 
    c.setStatusFlag(Core::StatusFlag::Z, v==0); c.setStatusFlag(Core::StatusFlag::N, (v&0x80)!=0); } };

struct Op_TAY { static void exec(Core& c, uint8_t) { 
    uint8_t v = static_cast<uint8_t>(c.a.getValue()); 
    c.y.setValue(v); 
    c.setStatusFlag(Core::StatusFlag::Z, v==0); c.setStatusFlag(Core::StatusFlag::N, (v&0x80)!=0); } };

struct Op_TXA { static void exec(Core& c, uint8_t) { 
    uint8_t v = static_cast<uint8_t>(c.x.getValue()); 
    c.a.setValue(v); 
    c.setStatusFlag(Core::StatusFlag::Z, v==0); c.setStatusFlag(Core::StatusFlag::N, (v&0x80)!=0); } };

struct Op_TYA { static void exec(Core& c, uint8_t) { 
    uint8_t v = static_cast<uint8_t>(c.y.getValue()); 
    c.a.setValue(v); 
    c.setStatusFlag(Core::StatusFlag::Z, v==0); c.setStatusFlag(Core::StatusFlag::N, (v&0x80)!=0); } };

struct Op_TSX { static void exec(Core& c, uint8_t) { 
    uint8_t v = static_cast<uint8_t>(c.s.getValue()); 
    c.x.setValue(v); 
    c.setStatusFlag(Core::StatusFlag::Z, v==0); c.setStatusFlag(Core::StatusFlag::N, (v&0x80)!=0); } };

struct Op_TXS { static void exec(Core& c, uint8_t) { c.s.setValue(c.x.getValue()); } };

struct Op_CLC { static void exec(Core& c, uint8_t) { c.setStatusFlag(Core::StatusFlag::C, false); } };
struct Op_SEC { static void exec(Core& c, uint8_t) { c.setStatusFlag(Core::StatusFlag::C, true); } };
struct Op_CLI { static void exec(Core& c, uint8_t) { c.setStatusFlag(Core::StatusFlag::I, false); } };
struct Op_SEI { static void exec(Core& c, uint8_t) { c.setStatusFlag(Core::StatusFlag::I, true); } };
struct Op_CLV { static void exec(Core& c, uint8_t) { c.setStatusFlag(Core::StatusFlag::V, false); } };
struct Op_CLD { static void exec(Core& c, uint8_t) { c.setStatusFlag(Core::StatusFlag::D, false); } };
struct Op_SED { static void exec(Core& c, uint8_t) { c.setStatusFlag(Core::StatusFlag::D, true); } };

// =============================================================
// OPERATIONS: CONTROL FLOW & STACK
// =============================================================

// Helper for Stack Pushes
struct StackOps {
    static void push(Core& c, uint8_t val) {
        uint8_t sp = static_cast<uint8_t>(c.s.getValue());
        c.write(0x0100 | sp, val);
        c.s.setValue(sp - 1);
    }
    static uint8_t pop(Core& c) {
        uint8_t sp = static_cast<uint8_t>(c.s.getValue()) + 1;
        c.s.setValue(sp);
        return c.read(0x0100 | sp);
    }
};

struct Op_PHA { static void exec(Core& c, uint8_t) { StackOps::push(c, static_cast<uint8_t>(c.a.getValue())); } };

struct Op_PHP { static void exec(Core& c, uint8_t) { 
    // PHP sets Bit 4 (Break) AND Bit 5 (Unused) when pushing to stack
    StackOps::push(c, static_cast<uint8_t>(c.p.getValue()) | 0x30); 
} };

struct Op_PLA { static void exec(Core& c, uint8_t) { 
    uint8_t v = StackOps::pop(c);
    c.a.setValue(v);
    c.setStatusFlag(Core::StatusFlag::Z, v==0); 
    c.setStatusFlag(Core::StatusFlag::N, (v&0x80)!=0);
} };

struct Op_PLP { static void exec(Core& c, uint8_t) { 
    // PLP ignores Bit 4 and Bit 5 from the stack value
    uint8_t v = StackOps::pop(c);
    uint8_t current = static_cast<uint8_t>(c.p.getValue());
    // Keep bits 5 and 4 of current, overwrite rest from stack
    // (Actually 6502 implementation details vary, but bit 5 is usually ignored on set)
    // The safest emulator way is: P = (stack & ~0x30) | (current & 0x30) OR just raw set but ignore effect of B
    c.p.setValue((v & ~0x30) | (current & 0x30));
} };

// JMP Absolute: Sets PC to the effective address
struct Op_JMP {
    static void exec(Core& c, uint8_t) { (void)c; /* No-op here, logic handled in mode? No. */ }
};
// Specialized Template for JMP to handle "Get Address and Set PC"
template <typename MODE>
void ExecJMP(Core& core) {
    uint16_t target = MODE::getAddr(core);
    core.pc.setValue(target);
}

struct Op_JSR {
    static void exec(Core& c, uint8_t) { (void)c; }
};
// Specialized JSR Logic
// JSR pushes PC+2 (which is effectively the address of the last byte of the JSR instruction)
void ExecJSR(Core& core) {
    uint16_t target = core.fetchWord(); // This fetches the target address (PC is now JSR + 3)
    // We need to push PC - 1 (The address of the 2nd byte of the address)
    uint16_t pushPC = static_cast<uint16_t>(core.pc.getValue()) - 1;
    StackOps::push(core, (pushPC >> 8) & 0xFF);
    StackOps::push(core, pushPC & 0xFF);
    core.pc.setValue(target);
}

struct Op_RTS {
    static void exec(Core& core, uint8_t) {
        uint16_t low = StackOps::pop(core);
        uint16_t high = StackOps::pop(core);
        uint16_t addr = (high << 8) | low;
        core.pc.setValue(addr + 1); // RTS jumps to Popped Address + 1
    }
};

struct Op_RTI {
    static void exec(Core& core, uint8_t) {
        // Pop P (ignore bits 4/5)
        uint8_t flags = StackOps::pop(core);
        uint8_t current = static_cast<uint8_t>(core.p.getValue());
        core.p.setValue((flags & ~0x30) | (current & 0x30));
        
        // Pop PC
        uint16_t low = StackOps::pop(core);
        uint16_t high = StackOps::pop(core);
        core.pc.setValue((high << 8) | low);
    }
};

struct Op_BRK {
    static void exec(Core& core, uint8_t) {
        core.fetch(); // Consume padding
        uint16_t pc = static_cast<uint16_t>(core.pc.getValue());
        
        StackOps::push(core, (pc >> 8) & 0xFF);
        StackOps::push(core, pc & 0xFF);
        StackOps::push(core, static_cast<uint8_t>(core.p.getValue()) | 0x30); // B-Flag set
        
        core.setStatusFlag(Core::StatusFlag::I, true);
        uint16_t vector = core.read(0xFFFE) | (core.read(0xFFFF) << 8);
        core.pc.setValue(vector);
    }
};

// =============================================================
// ILLEGAL OPERATIONS
// =============================================================

struct Op_JAM {
    static void exec(Core& core, uint8_t) {
        // Hardware Freeze: The CPU stops incrementing PC and effectively hangs.
        // We simulate this by constantly rewinding the PC so it executes this instruction forever.
        // fetch() moved PC forward by 1; we move it back by 1.
        core.pc.setValue(core.pc.getValue() - 1);
    }
};

struct Op_NOP { static void exec(Core&, uint8_t) { /* Do nothing */ } };


template <typename MODE, typename RMW_OP, typename ALU_OP>
void ExecRMW_ALU(Core& core) {
    uint16_t addr = MODE::getAddr(core);    // Reuse Addressing Mode
    uint8_t val = core.read(addr);          // Read
    
    // REUSE 1: The Modify Logic (e.g., Op_ASL::calc)
    uint8_t res = RMW_OP::calc(core, val);
    
    core.write(addr, res);                  // Write
    
    // REUSE 2: The ALU Logic (e.g., Op_ORA::exec)
    ALU_OP::exec(core, res);
}

// 1. LAX: Composition of LDA and LDX
// We just call the standard Exec logic for both!
struct Op_LAX {
    static void exec(Core& core, uint8_t v) {
        Op_LDA::exec(core, v); // Reuses your existing LDA logic (loads A, sets flags)
        Op_LDX::exec(core, v); // Reuses your existing LDX logic (loads X, sets flags)
    }
};

// 2. SAX: Composition of Src_A and Src_X
// We reuse the existing "Get Register" helpers.
struct Src_AX {
    static uint8_t get(Core& core) {
        // Reuses Src_A and Src_X logic
        return Src_A::get(core) & Src_X::get(core);
    }
};

// Decrement Y Register
struct Op_DEY { 
    static void exec(Core& c, uint8_t) {
        // Decrement and wrap (0x00 -> 0xFF)
        uint8_t v = static_cast<uint8_t>(c.y.getValue()) - 1;
        c.y.setValue(v);
        c.setStatusFlag(Core::StatusFlag::Z, v == 0); 
        c.setStatusFlag(Core::StatusFlag::N, (v & 0x80) != 0);
    } 
};

// Increment Y Register
struct Op_INY { 
    static void exec(Core& c, uint8_t) {
        uint8_t v = static_cast<uint8_t>(c.y.getValue()) + 1;
        c.y.setValue(v);
        c.setStatusFlag(Core::StatusFlag::Z, v == 0); 
        c.setStatusFlag(Core::StatusFlag::N, (v & 0x80) != 0);
    } 
};

// Increment X Register
// Note: DEX is in Column A, but INX is here in Column 8.
struct Op_INX { 
    static void exec(Core& c, uint8_t) {
        uint8_t v = static_cast<uint8_t>(c.x.getValue()) + 1;
        c.x.setValue(v);
        c.setStatusFlag(Core::StatusFlag::Z, v == 0); 
        c.setStatusFlag(Core::StatusFlag::N, (v & 0x80) != 0);
    } 
};

// Decrement X Register
struct Op_DEX { 
    static void exec(Core& c, uint8_t) {
        uint8_t v = static_cast<uint8_t>(c.x.getValue()) - 1;
        c.x.setValue(v);
        c.setStatusFlag(Core::StatusFlag::Z, v == 0); 
        c.setStatusFlag(Core::StatusFlag::N, (v & 0x80) != 0);
    } 
};

// 1. ANC (AND + Carry) - 0x0B, 0x2B
// Performs AND #imm, then sets Carry equal to Bit 7 (Negative Flag).
struct Op_ANC {
    static void exec(Core& core, uint8_t v) {
        Op_AND::exec(core, v); // A = A & v; Sets N, Z
        // C = Bit 7 of result (which is N)
        core.setStatusFlag(Core::StatusFlag::C, core.getStatusFlag(Core::StatusFlag::N));
    }
};

// 2. ALR (AND + LSR) - 0x4B
// Performs AND #imm, then LSR A.
struct Op_ALR {
    static void exec(Core& core, uint8_t v) {
        uint8_t a = static_cast<uint8_t>(core.a.getValue());
        a &= v; // AND
        // LSR Logic: Carry = Bit 0, Result = A >> 1
        core.setStatusFlag(Core::StatusFlag::C, (a & 0x01) != 0);
        a >>= 1;
        core.a.setValue(a);
        core.setStatusFlag(Core::StatusFlag::Z, a == 0);
        core.setStatusFlag(Core::StatusFlag::N, false); // LSR always clears N
    }
};

/// 3. ARR (AND + ROR) - 0x6B
// Performs AND #imm, then ROR A.
// Has unique Decimal Mode fixups on NMOS 6502.
struct Op_ARR {
    static void exec(Core& core, uint8_t v) {
        uint8_t a = static_cast<uint8_t>(core.a.getValue());
        uint8_t temp = a & v; // Perform AND first
        
        // ROR Logic: Shift Right, shifting Carry into Bit 7
        bool oldCarry = core.getStatusFlag(Core::StatusFlag::C);
        uint8_t result = (temp >> 1) | (oldCarry ? 0x80 : 0);
        
        // Standard Flags (N, Z) based on the BINARY result
        core.setStatusFlag(Core::StatusFlag::Z, result == 0);
        core.setStatusFlag(Core::StatusFlag::N, (result & 0x80) != 0);
        
        // V Flag: Bit 6 XOR Bit 5 of the result
        bool bit6 = (result & 0x40) != 0;
        bool bit5 = (result & 0x20) != 0;
        core.setStatusFlag(Core::StatusFlag::V, bit6 ^ bit5);

        if (core.getStatusFlag(Core::StatusFlag::D)) {
            // --- DECIMAL MODE FIXUP ---
            // The checks use the *Intermediate AND* value (temp), not the ROR result.
            
            // Low Nibble Fix:
            // Logic: If (LowNibble + (LowNibble & 1)) > 5
            uint8_t low = temp & 0x0F;
            if ((low + (low & 1)) > 5) {
                result = (result & 0xF0) | ((result + 6) & 0x0F);
            }

            // High Nibble Fix & Carry Flag:
            // Logic: If (HighNibble + (HighNibble & 0x10)) > 0x50
            uint8_t high = temp & 0xF0;
            if ((high + (high & 0x10)) > 0x50) {
                result = (result + 0x60) & 0xFF; // Apply fix
                core.setStatusFlag(Core::StatusFlag::C, true); // Carry Set
            } else {
                core.setStatusFlag(Core::StatusFlag::C, false); // Carry Clear
            }
            
            core.a.setValue(result);
            
        } else {
            // --- BINARY MODE ---
            // C Flag is simply Bit 6 of the result (Hardware quirk of ARR)
            core.setStatusFlag(Core::StatusFlag::C, bit6);
            core.a.setValue(result);
        }
    }
};

// 4. XAA / ANE (Transmit X to A, AND #imm) - 0x8B
// This opcode is unstable. 
// The formula that satisfies the Dormann test suite is: A = (A | 0xEE) & X & Imm
struct Op_XAA {
    static void exec(Core& core, uint8_t v) {
        uint8_t a = static_cast<uint8_t>(core.a.getValue());
        uint8_t x = static_cast<uint8_t>(core.x.getValue());
        
        // The magic 0xEE comes from analog settling behaviors on the NMOS 6502 data bus.
        uint8_t res = (a | 0xEE) & x & v;
        
        core.a.setValue(res);
        core.setStatusFlag(Core::StatusFlag::Z, res == 0);
        core.setStatusFlag(Core::StatusFlag::N, (res & 0x80) != 0);
    }
};

// 5. AXS / SBX (CMP X with AND - Subtract) - 0xCB
// Computes (A & X) - imm. Stores result in X. Sets flags.
struct Op_AXS {
    static void exec(Core& core, uint8_t v) {
        uint8_t a_x = static_cast<uint8_t>(core.a.getValue() & core.x.getValue());
        uint8_t diff = a_x - v;
        
        // C is set if (A & X) >= v (No borrow)
        core.setStatusFlag(Core::StatusFlag::C, a_x >= v);
        
        core.x.setValue(diff);
        core.setStatusFlag(Core::StatusFlag::Z, diff == 0);
        core.setStatusFlag(Core::StatusFlag::N, (diff & 0x80) != 0);
    }
};

// 6. LAS (Load Accumulator, X, Stack) - 0xBB
// A = X = S = (Mem & S)
struct Op_LAS {
    static void exec(Core& core, uint8_t v) {
        uint8_t sp = static_cast<uint8_t>(core.s.getValue());
        uint8_t res = v & sp;
        
        core.a.setValue(res);
        core.x.setValue(res);
        core.s.setValue(res);
        
        core.setStatusFlag(Core::StatusFlag::Z, res == 0);
        core.setStatusFlag(Core::StatusFlag::N, (res & 0x80) != 0);
    }
};

// Custom Logic for TAS (0x9B) - Absolute, Y
// Updates Stack Pointer AND writes to memory with specific glitch behavior.
void ExecTAS(Core& core) {
    // 1. Fetch Base Address manually (so we have the High Byte)
    uint16_t baseAddr = core.fetchWord(); 
    
    // 2. Calculate Effective Address
    uint8_t y = static_cast<uint8_t>(core.y.getValue());
    uint16_t effectiveAddr = baseAddr + y;
    
    // 3. Update Stack Pointer (A & X)
    uint8_t a = static_cast<uint8_t>(core.a.getValue());
    uint8_t x = static_cast<uint8_t>(core.x.getValue());
    uint8_t sp = a & x;
    core.s.setValue(sp);
    
    // 4. Calculate Value to Write
    // Formula: SP & (BaseHigh + 1)
    uint8_t baseHigh = (baseAddr >> 8) & 0xFF;
    uint8_t val = sp & (baseHigh + 1);
    
    // 5. The Address Glitch
    // If the Page Boundary was crossed, the High Byte is corrupted by ANDing it with the Value.
    if ((effectiveAddr >> 8) != baseHigh) {
        uint8_t effectiveHigh = (effectiveAddr >> 8) & 0xFF;
        effectiveAddr = ((effectiveHigh & val) << 8) | (effectiveAddr & 0xFF);
    }
    
    core.write(effectiveAddr, val);
}

// Custom Logic for SHY (0x9C) - Absolute, X
// Glitch: If page boundary crossed, TargetHigh = TargetHigh & ((BaseHigh + 1) & Y)
void ExecSHY(Core& core) {
    // 1. Fetch Base Address
    uint16_t baseAddr = core.fetchWord();
    
    // 2. Calculate Indexed Address
    uint8_t x = static_cast<uint8_t>(core.x.getValue());
    uint16_t effectiveAddr = baseAddr + x;
    
    // 3. Calculate Value to Write: Y & (BaseHigh + 1)
    uint8_t y = static_cast<uint8_t>(core.y.getValue());
    uint8_t baseHigh = (baseAddr >> 8) & 0xFF;
    uint8_t val = y & (baseHigh + 1);
    
    // 4. The Glitch: Check for Page Crossing
    // If the High Byte of the effective address differs from the Base Address,
    // the hardware fails to drive the correct high byte.
    // Instead, the High Byte becomes (EffectiveHigh & Value).
    if ((effectiveAddr >> 8) != baseHigh) {
        uint8_t effectiveHigh = (effectiveAddr >> 8) & 0xFF;
        effectiveAddr = ((effectiveHigh & val) << 8) | (effectiveAddr & 0xFF);
    }
    
    core.write(effectiveAddr, val);
}

// Custom Logic for SHX (0x9E) - Absolute, Y
// Same glitch, but uses X register and Y index.
void ExecSHX(Core& core) {
    uint16_t baseAddr = core.fetchWord();
    
    uint8_t y = static_cast<uint8_t>(core.y.getValue());
    uint16_t effectiveAddr = baseAddr + y;
    
    uint8_t x = static_cast<uint8_t>(core.x.getValue());
    uint8_t baseHigh = (baseAddr >> 8) & 0xFF;
    uint8_t val = x & (baseHigh + 1);
    
    if ((effectiveAddr >> 8) != baseHigh) {
        uint8_t effectiveHigh = (effectiveAddr >> 8) & 0xFF;
        effectiveAddr = ((effectiveHigh & val) << 8) | (effectiveAddr & 0xFF);
    }
    
    core.write(effectiveAddr, val);
}

// Custom Logic for SHA / AXA (0x9F) - Absolute, Y
// Stores (A & X) & (HighByte + 1)
// Implements the "High Byte Malfunction" glitch on page crossing.
void ExecSHA(Core& core) {
    // 1. Fetch Base Address manually to preserve the original High Byte
    uint16_t baseAddr = core.fetchWord(); 
    uint8_t baseHigh = (baseAddr >> 8) & 0xFF;
    
    // 2. Calculate Effective Address
    uint8_t y = static_cast<uint8_t>(core.y.getValue());
    uint16_t effectiveAddr = baseAddr + y;
    
    // 3. Calculate Value to Write
    // Formula: (A & X) & (BaseHigh + 1)
    uint8_t a = static_cast<uint8_t>(core.a.getValue());
    uint8_t x = static_cast<uint8_t>(core.x.getValue());
    uint8_t val = (a & x) & (baseHigh + 1);
    
    // 4. The Address Glitch
    // If the High Byte changed (Page Crossing), the target address is corrupted.
    // The High Byte becomes (TargetHigh & Value).
    if ((effectiveAddr >> 8) != baseHigh) {
        uint8_t effectiveHigh = (effectiveAddr >> 8) & 0xFF;
        effectiveAddr = ((effectiveHigh & val) << 8) | (effectiveAddr & 0xFF);
    }
    
    core.write(effectiveAddr, val);
}

// Custom Logic for ATX / LXA (0xAB) - Unstable
// Load A and X with (A | 0xEE) & Immediate
struct Op_ATX {
    static void exec(Core& core, uint8_t v) {
        uint8_t a = static_cast<uint8_t>(core.a.getValue());
        
        // The Magic Constant 0xEE strikes again!
        uint8_t res = (a | 0xEE) & v;
        
        core.a.setValue(res);
        core.x.setValue(res);
        
        core.setStatusFlag(Core::StatusFlag::Z, res == 0);
        core.setStatusFlag(Core::StatusFlag::N, (res & 0x80) != 0);
    }
};

// Custom Logic for SHA / AXA (0x93) - (Indirect), Y
// This is the most complex illegal store.
void ExecSHA_INDY(Core& core) {
    // 1. Fetch Pointer Address from Zero Page
    uint8_t ptrAddr = core.fetch();
    
    // 2. Read the Effective Address from the ZP Pointer
    uint16_t low = core.read(ptrAddr);
    uint16_t high = core.read(static_cast<uint8_t>(ptrAddr + 1));
    uint16_t baseAddr = (high << 8) | low;
    
    // 3. Calculate Effective Address (Base + Y)
    uint8_t y = static_cast<uint8_t>(core.y.getValue());
    uint16_t effectiveAddr = baseAddr + y;
    
    // 4. Calculate Value to Write
    // Formula: (A & X) & (HighByteOfBase + 1)
    uint8_t a = static_cast<uint8_t>(core.a.getValue());
    uint8_t x = static_cast<uint8_t>(core.x.getValue());
    uint8_t baseHigh = (baseAddr >> 8) & 0xFF;
    
    uint8_t val = (a & x) & (baseHigh + 1);
    
    // 5. The Glitch: Check for Page Crossing
    // If Effective High != Base High, the High Byte gets corrupted.
    if ((effectiveAddr >> 8) != baseHigh) {
        uint8_t effectiveHigh = (effectiveAddr >> 8) & 0xFF;
        effectiveAddr = ((effectiveHigh & val) << 8) | (effectiveAddr & 0xFF);
    }
    
    core.write(effectiveAddr, val);
}