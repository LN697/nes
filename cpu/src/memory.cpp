#include "logger.hpp"

#include "memory.hpp"
#include "stdexcept"

Memory::Memory(int size_bytes) : size_bytes(size_bytes) {
    memory.resize(size_bytes);
    log("MEMORY", "Memory initialized with size: " + std::to_string(64 * KB) + " bytes.");
}

Memory::~Memory() = default;