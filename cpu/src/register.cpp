#include "register.hpp"
#include "logger.hpp"

Register::Register(RegType type, const std::string* name) : type(type), name(name) {
    value = 0x0;
    log("REGISTER", "Initialized register " + *name + " of type " + std::to_string(static_cast<int>(type)) + " with initial value 0.");
}

Register::~Register() = default;

const std::string& Register::getName() const {
    return *name;
}

RegType Register::getType() const {
    return type;
}

std::uint64_t Register::getValue() const {
    return value;
}

void Register::setBit(int position) {
    value |= (1ULL << position);
    log("REGISTER", "Set bit " + std::to_string(position) + " of register " + *name + ". New value: " + std::to_string(value) + ".");
}

void Register::clearBit(int position) {
    value &= ~(1ULL << position);
    log("REGISTER", "Cleared bit " + std::to_string(position) + " of register " + *name + ". New value: " + std::to_string(value) + ".");
}

bool Register::getBit(int position) const {
    return (value >> position) & 1ULL;
}

void Register::setValue(std::uint64_t val) {
    value = val;
}