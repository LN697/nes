#include "logger.hpp"

#include "memory.hpp"

Memory::Memory(int size_bytes) : size_bytes(size_bytes) {
    memory.resize(size_bytes);
    log("MEMORY", "Memory initialized with size: " + std::to_string(size_bytes) + " bytes.");
}

Memory::~Memory() = default;

std::uint8_t Memory::getMemory(int address) const {
    if (address < 0 || address >= size_bytes) {
        log("MEMORY", "Attempted to read from invalid memory address: " + std::to_string(address));
        log("MEMORY", "FAIL");
        return 0;
    }
    return memory[address];
}

bool Memory::setMemory(int address, std::uint8_t value) {
    if (address < 0 || address >= size_bytes) {
        log("MEMORY", "Attempted to write to invalid memory address: " + std::to_string(address));
        return false;
    }
    memory[address] = value;
    return true;
}

int Memory::getSize() const {
    return size_bytes;
}