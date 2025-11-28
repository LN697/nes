#pragma once

#include "core_base.hpp"
#include "register.hpp"

#include <array>
#include <functional>

class Core;

using OpHandler = void(*)(Core&);

class Core : public CoreBase {
    public:
        Register a;
        Register x;
        Register y;
        Register s;
        Register pc;
        Register p;

        std::array<OpHandler, 256> instr_table;

    public:
        Core(Memory* memory);

        ~Core() override;

        void step(int ops) override;
        void run() override;
        void init() override;

        std::uint8_t read(std::uint16_t address) const;
        void write(std::uint16_t address, std::uint8_t value);
        std::uint8_t fetch();
        std::uint16_t fetchWord();

        enum class StatusFlag {
            C = 0, // Carry
            Z = 1, // Zero
            I = 2, // Interrupt Disable
            D = 3, // Decimal (ignored on NES)
            B = 4, // Break
            U = 5, // Unused
            V = 6, // Overflow
            N = 7  // Negative
        };

        void setStatusFlag(StatusFlag flag, bool value);
        bool getStatusFlag(StatusFlag flag) const;
};