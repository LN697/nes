#pragma once

#include <cstdint>
#include <string>
#include <array>
#include <functional>

#include "register.hpp"
#include "logger.hpp"
#include "bus.hpp"

enum class Phase { STANDBY, FETCH, OPR_FETCH, READ, OPERATION, WRITE, INTERRUPT, ERROR };

class Core;

using OpHandler = std::function<void(Core&)>;

class Core {
    private:
        Bus* bus;
        Phase core_phase;

        // Variables
        uint8_t opcode, sp, status;
        uint16_t pc_val, lo, hi;

    public:
        Register a;
        Register x;
        Register y;
        Register s;
        Register pc;
        Register p;
        
        std::array<OpHandler, 256> instr_table;
        
        enum class StatusFlag {
            C = 0, // Carry
            Z = 1, // Zero
            I = 2, // Interrupt Disable
            D = 3, // Decimal
            B = 4, // Break
            U = 5, // Unused
            V = 6, // Overflow
            N = 7  // Negative
        };

        explicit Core(Bus* bus);
        virtual ~Core();

        void init();
        void step();
        
        std::uint8_t read(std::uint16_t address) const;
        void write(std::uint16_t address, std::uint8_t value);
        
        std::uint8_t fetch();
        std::uint16_t fetchWord();
        
        void setStatusFlag(StatusFlag flag, bool value);
        bool getStatusFlag(StatusFlag flag) const;
        
        int core_id = 0;
        int last_cycles = 0;
};