#pragma once

#include "cstdint"
#include "vector"


// This class is a byte addressed memory structure.
class Memory {
    private:
        std::vector<std::uint8_t>   memory;
        int                         size_bytes;
    public:
        Memory(int size_bytes);
        ~Memory();

        bool setMemory(int address, std::uint8_t value);
        std::uint8_t getMemory(int address) const;
        int getSize() const;

};