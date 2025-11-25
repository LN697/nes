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
    // Create a mask for the valid bit range of the register
    // For a size of 8, the mask is 0xFF. For 16, 0xFFFF, etc.
    std::uint64_t max_val = (size >= 64) ? std::numeric_limits<std::uint64_t>::max() : ((1ULL << size) - 1);

    if (val > max_val) {
        throw std::out_of_range("Value " + std::to_string(val) + " exceeds register " + *name + " size of " + std::to_string(size) + " bits (max value: " + std::to_string(max_val) + ").");
    }
    value = val;
    log("REGISTER", "Set value of register " + *name + ". New value: " + std::to_string(value) + ".");
}

void Register::andValue(std::uint64_t val) {
    // Ensure the `and` operation also respects the size of the register.
    // The result should not exceed the max_val for the register's size.
    std::uint64_t max_val = (size >= 64) ? std::numeric_limits<std::uint64_t>::max() : ((1ULL << size) - 1);
    std::uint64_t result = value & val;

    if (result > max_val) {
        // This case should ideally not be hit if `val` and `value` are already within bounds
        // but as a safeguard, we could truncate or throw.
        // For now, let's just log a warning or adjust if this becomes an issue.
        log("WARN", "AND operation resulted in a value exceeding register size, but will be masked. Result: " + std::to_string(result));
    }
    value = result & max_val; // Ensure the value remains within the register's bit size
    log("REGISTER", "Performed AND operation on register " + *name + ". New value: " + std::to_string(value) + ".");
}