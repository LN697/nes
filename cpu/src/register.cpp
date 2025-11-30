#include "register.hpp"
#include "logger.hpp"
#include <limits>

Register::Register(RegType type, const std::string* name, std::uint8_t size) : type(type), name(name), size(size) {
    value = 0x0;
    log("REGISTER", "Initialized register " + *name + " of type " + std::to_string(static_cast<int>(type)) + " with initial value 0 and size " + std::to_string(size) + ".");
}

Register::~Register() = default;

RegType Register::getType() const { return type; }
const std::string& Register::getName() const { return *name; }
std::uint8_t Register::getSize() const { return size; }
std::uint64_t Register::getValue() const { return value; }

void Register::setValue(std::uint64_t val) {
    std::uint64_t mask = (size >= 64) ? std::numeric_limits<std::uint64_t>::max() : ((1ULL << size) - 1);
    value = val & mask;
    
    // log("REGISTER", "Set " + *name + " to " + std::to_string(value));
}

void Register::setBit(int position) {
    if (position < size) value |= (1ULL << position);
}

void Register::clearBit(int position) {
    if (position < size) value &= ~(1ULL << position);
}

bool Register::getBit(int position) const {
    if (position >= size) return false;
    return (value & (1ULL << position)) != 0;
}

void Register::andValue(std::uint64_t val) {
    value &= val;
}