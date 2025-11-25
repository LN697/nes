#pragma once

#include <cstdint>
#include "core.hpp"

// 1. IMPLICIT / IMPLIED (e.g., TAX, INX)
// Operates on registers, no memory address needed.
struct IMP {
    static constexpr bool is_implied = true;
    static uint16_t getAddr(Core& core) { (void)core; return 0; } 
};

// 2. IMMEDIATE (e.g., LDA #$10)
// The data is at the PC. We return the PC as the address, then increment it.
struct IMM {
    static constexpr bool is_implied = true;
    static std::uint16_t getAddr(Core& core) {
        std::uint16_t address = static_cast<std::uint16_t>(core.pc.getValue());
        core.pc.setValue(static_cast<std::uint64_t>(address + 1));
        return address;
    }
};

// 3. ZERO PAGE (e.g., LDA $00)
// Address is in the first 256 bytes.
struct ZP {
    static constexpr bool is_implied = true;
    static std::uint16_t getAddr(Core& core) {
        return core.fetch();
    }
};

// 4. ZERO PAGE, X (e.g., LDA $00,X)
// Address is ZP + X (Wraps within page 0)
struct ZPX {
    static constexpr bool is_implied = false;
    static uint16_t getAddr(Core& core) { 
        return static_cast<uint16_t>((core.fetch() + static_cast<uint8_t>(core.x.getValue())) & 0xFF);
    }
};

// 5. ZERO PAGE, Y (e.g., LDX $00,Y)
struct ZPY {
    static constexpr bool is_implied = false;
    static uint16_t getAddr(Core& core) { 
        return static_cast<uint16_t>((core.fetch() + static_cast<uint8_t>(core.y.getValue())) & 0xFF);
    }
};

// 6. ABSOLUTE (e.g., LDA $1234)
struct ABS {
    static constexpr bool is_implied = false;
    static uint16_t getAddr(Core& core) { return core.fetchWord(); }
};

// 7. ABSOLUTE, X (e.g., LDA $1234,X)
struct ABSX {
    static constexpr bool is_implied = false;
    static uint16_t getAddr(Core& core) { return static_cast<uint16_t>(core.fetchWord() + static_cast<uint16_t>(core.x.getValue())); }
};

// 8. ABSOLUTE, Y (e.g., LDA $1234,Y)
struct ABSY {
    static constexpr bool is_implied = false;
    static uint16_t getAddr(Core& core) { return static_cast<uint16_t>(core.fetchWord() + static_cast<uint16_t>(core.y.getValue())); }
};

// 9. INDIRECT (e.g., JMP ($1234))
// Specifically for JMP. Reads a pointer from memory.
// Note: Has a famous hardware bug on page boundaries not emulated here for brevity.
struct IND {
    static constexpr bool is_implied = false;
    static uint16_t getAddr(Core& core) {
        uint16_t ptr = core.fetchWord();
        return core.read(ptr) | (core.read(ptr + 1) << 8);
    }
};

// 10. INDEXED INDIRECT (Indirect, X) (e.g., LDA ($20,X))
// "Pre-indexed". Add X to ZP byte, then read pointer.
struct INDX {
    static constexpr bool is_implied = false;
    static uint16_t getAddr(Core& core) {
        uint8_t ptr = static_cast<uint8_t>((core.fetch() + static_cast<uint8_t>(core.x.getValue())) & 0xFF); // Wrap ZP
        // Read 16-bit effective address from ZP, wrapping inside ZP
        uint16_t low = core.read(ptr);
        uint16_t high = core.read((ptr + 1) & 0xFF);
        return (high << 8) | low;
    }
};

// 11. INDIRECT INDEXED (Indirect, Y) (e.g., LDA ($20),Y)
// "Post-indexed". Read pointer from ZP, then add Y.
struct INDY {
    static constexpr bool is_implied = false;
    static uint16_t getAddr(Core& core) {
        uint8_t ptr = core.fetch();
        uint16_t low = core.read(ptr);
        uint16_t high = core.read((ptr + 1) & 0xFF);
        return static_cast<uint16_t>(((high << 8) | low) + static_cast<uint16_t>(core.y.getValue()));
    }
};

// 12. RELATIVE (e.g., BEQ *+4)
// Used for Branching. Returns the target PC.
struct REL {
    static constexpr bool is_implied = false;
    static uint16_t getAddr(Core& core) {
        int8_t offset = static_cast<int8_t>(core.fetch()); // Signed!
        return static_cast<uint16_t>(core.pc.getValue() + offset);
    }
};

// 13. ACCUMULATOR (e.g., ASL A)
// Logic acts on Register A, not memory.
struct ACC {
    static constexpr bool is_implied = true; 
    // Uses A, no address needed.
    static uint16_t getAddr(Core& core) { (void)core; return 0; }
};


// Base Template for READ instructions
template <typename MODE, typename OPERATION>
void ExecRead(Core& core) {
    uint16_t addr = MODE::getAddr(core);
    uint8_t  val  = core.read(addr);
    OPERATION::exec(core, val);
}

// Logic implementations
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
struct Op_ADC { 
    static void exec(Core& core, uint8_t v) { 
        uint8_t a_val = static_cast<uint8_t>(core.a.getValue());
        uint8_t carry = core.getStatusFlag(Core::StatusFlag::C) ? 1 : 0;

        uint16_t sum = a_val + v + carry;

        core.setStatusFlag(Core::StatusFlag::C, sum > 0xFF);
        // Overflow is set if (N_A == N_M) and (N_A != N_R)
        // N_A is the sign of accumulator, N_M is sign of memory, N_R is sign of result
        core.setStatusFlag(Core::StatusFlag::V, (((a_val ^ sum) & (v ^ sum) & 0x80) != 0));

        core.a.setValue(static_cast<uint8_t>(sum));
        uint8_t result8 = static_cast<uint8_t>(sum);
        core.setStatusFlag(Core::StatusFlag::Z, result8 == 0);
        core.setStatusFlag(Core::StatusFlag::N, (result8 & 0x80) != 0);
    } 
};
struct Op_AND { 
    static void exec(Core& core, uint8_t v) { 
        auto newv = core.a.getValue() & static_cast<std::uint64_t>(v);
        core.a.setValue(newv);
        core.setStatusFlag(Core::StatusFlag::Z, static_cast<uint8_t>(newv) == 0);
        core.setStatusFlag(Core::StatusFlag::N, (static_cast<uint8_t>(newv) & 0x80) != 0);
    } 
};

// Placeholder for BRK - mainly for setting the B flag
struct Op_BRK {
    static void exec(Core& core, uint8_t v) {
        // BRK instruction sets the B flag to 1, pushes PC and P to stack
        // The actual BRK interrupt handling will be in Core::step()
        (void)v;
        core.setStatusFlag(Core::StatusFlag::B, true);
    }
};


// Base Template for WRITE instructions
template <typename MODE, typename SOURCE>
void ExecWrite(Core& core) {
    uint16_t addr = MODE::getAddr(core);
    uint8_t  val  = SOURCE::get(core);
    core.write(addr, val);
}

// Register Sources
struct Src_A { static uint8_t get(Core& core) { return static_cast<uint8_t>(core.a.getValue()); } };
struct Src_X { static uint8_t get(Core& core) { return static_cast<uint8_t>(core.x.getValue()); } };
struct Src_Y { static uint8_t get(Core& core) { return static_cast<uint8_t>(core.y.getValue()); } };


template <typename MODE, typename OPERATION>
void ExecRMW(Core& core) {
    // 1. Accumulator Mode Special Case
    if constexpr (std::is_same_v<MODE, ACC>) {
        uint8_t result = OPERATION::calc(core, static_cast<uint8_t>(core.a.getValue()));
        core.a.setValue(result);
    } 
    // 2. Memory Mode
    else {
        uint16_t addr = MODE::getAddr(core);
        uint8_t  val  = core.read(addr);
        uint8_t  res  = OPERATION::calc(core, val);
        core.write(addr, res); // Write back
    }
}

// The Calculation Logic
struct Op_INC { 
    static uint8_t calc(Core& core, uint8_t v) { 
        uint8_t r = v + 1;
        core.setStatusFlag(Core::StatusFlag::Z, r == 0);
        core.setStatusFlag(Core::StatusFlag::N, (r & 0x80) != 0);
        return r; 
    } 
};
struct Op_ASL { 
    static uint8_t calc(Core& core, uint8_t v) { 
        // Set Carry = old bit 7, shift left
        core.setStatusFlag(Core::StatusFlag::C, (v & 0x80) != 0);
        uint8_t r = v << 1; 
        core.setStatusFlag(Core::StatusFlag::Z, r == 0);
        core.setStatusFlag(Core::StatusFlag::N, (r & 0x80) != 0);
        return r; 
    } 
};