#include "register.hpp"
#include "logger.hpp"
#include <limits>

Register::Register(RegType type, const std::string* name, std::uint8_t size) : type(type), name(name), size(size) {
    value = 0x0;
    log("REGISTER", "Initialized register " + *name + " of type " + std::to_string(static_cast<int>(type)) + " with initial value 0 and size " + std::to_string(size) + ".");
}

Register::~Register() = default;

const std::string& Register::getName() const {
    return *name;
}

RegType Register::getType() const {
    return type;
}

std::uint8_t Register::getSize() const {
    return size;
}

std::uint64_t Register::getValue() const {
    return value;
}

void Register::setBit(int position) {
    if (position >= size) {
        throw std::out_of_range("Bit position out of bounds for register " + *name);
    }
    value |= (1ULL << position);
    log("REGISTER", "Set bit " + std::to_string(position) + " of register " + *name + ". New value: " + std::to_string(value) + ".");
}

void Register::clearBit(int position) {
    if (position >= size) {
        throw std::out_of_range("Bit position out of bounds for register " + *name);
    }
    value &= ~(1ULL << position);
    log("REGISTER", "Cleared bit " + std::to_string(position) + " of register " + *name + ". New value: " + std::to_string(value) + ".");
}

bool Register::getBit(int position) const {
    if (position >= size) {
        throw std::out_of_range("Bit position out of bounds for register " + *name);
    }
    return (value >> position) & 1ULL;
}

void Register::setValue(std::uint64_t val) {
    // 1. Calculate the Mask
    // (Optimization Tip: Calculate this once in the Constructor and store it as `uint64_t mask`)
    std::uint64_t mask = (size >= 64) ? std::numeric_limits<std::uint64_t>::max() : ((1ULL << size) - 1);

    // 2. The "Hardware" Logic
    // Instead of checking bounds, we force the value to fit.
    // If val is 0xFFFFFFFFFFFFFFFF (from 0 - 1 underflow) and size is 8 (mask 0xFF),
    // this operation turns it into 0x00000000000000FF.
    value = val & mask;

    // Optional: Log only if debug is enabled, otherwise this spams the console
    log("REGISTER", "Set " + *name + " to " + std::to_string(value));
}

void Register::andValue(std::uint64_t val) {
    // 1. Logic Check
    // A bitwise AND can NEVER increase a value. 
    // If 'value' is currently 255, 'value & anything' cannot be > 255.
    // So the bounds check here was actually mathematically impossible to trigger!
    
    value &= val; 
    
    // Safety mask (only needed if you suspect 'value' was already corrupt)
    std::uint64_t mask = (size >= 64) ? std::numeric_limits<std::uint64_t>::max() : ((1ULL << size) - 1);
    value &= mask;
    
    log("REGISTER", "AND " + *name + " result: " + std::to_string(value));
}