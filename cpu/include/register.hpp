#pragma once

#include "cstdint"
#include "string"

enum class RegType { MR, IR, PC, SR }; // Main Register, Instruction Register, Program Counter, Status Register

class Register {
    private:
        RegType type;
        const std::string* name;
        std::uint64_t value;

    public:
        Register(RegType type, const std::string* name);
        ~Register();

        RegType getType() const;
        const std::string& getName() const;

        std::uint64_t getValue() const;
        void setBit(int position);
        void clearBit(int position);
        bool getBit(int position) const;
        void setValue(std::uint64_t val);
};