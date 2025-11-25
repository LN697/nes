#pragma once

#include "cstdint"
#include "string"
#include "stdexcept" // Required for std::out_of_range

enum class RegType { MR, IR, PC, SR }; // Main Register, Instruction Register, Program Counter, Status Register

class Register {
    private:
        RegType type;
        const std::string* name;
        std::uint64_t value;
        std::uint8_t size; // Size of the register in bits

    public:
        Register(RegType type, const std::string* name, std::uint8_t size);
        ~Register();

        RegType getType() const;
        const std::string& getName() const;
        std::uint8_t getSize() const; // New getter for register size

        std::uint64_t getValue() const;
        void setBit(int position);
        void clearBit(int position);
        bool getBit(int position) const;
        void setValue(std::uint64_t val);
        void andValue(std::uint64_t val);
};