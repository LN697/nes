#include "core.hpp"
#include <string>

namespace {
    static const std::string a_name("Accumulator");
    static const std::string x_name("X index");
    static const std::string y_name("Y index");
    static const std::string s_name("Stack pointer");
    static const std::string pc_name("Program Counter");
    static const std::string p_name("Processor flags");
}

Core::Core(Memory* memory)
    : CoreBase(memory)
    , a(RegType::MR, &a_name, 8)  // 8-bit Accumulator
    , x(RegType::IR, &x_name, 8)  // 8-bit X index
    , y(RegType::IR, &y_name, 8)  // 8-bit Y index
    , s(RegType::IR, &s_name, 8)  // 8-bit Stack pointer
    , pc(RegType::PC, &pc_name, 16) // 16-bit Program Counter
    , p(RegType::SR, &p_name, 8)  // 8-bit Processor flags
{
    core_phase = Phase::STANDBY;

    log("CORE", "Core [" + std::to_string(core_id) + "] initialized in " + std::to_string(static_cast<int>(core_phase)) + " phase.");
}

Core::~Core() = default;

void Core::init() {
    pc.setValue(memory->getMemory(0x0000)); // Loading dummy data for testing
    log("CORE", "Core [" + std::to_string(core_id) + "] initialized. PC set to: " + std::to_string(pc.getValue()));
}

void Core::step() {
    if (core_phase != Phase::ERROR) {
        log("CORE", "Stepping core [" + std::to_string(core_id) + "]");
    } else {
        log("CORE", "Core [" + std::to_string(core_id) + "] halting due to error.");
        exit(1);
    }
}

void Core::run() {
    init();
    for (int i = 0x00 ; i < 0x0020; ++i) {
        step();
    }
}

std::uint8_t Core::read(std::uint16_t address) const {
    return memory->getMemory(static_cast<int>(address));
}

void Core::write(std::uint16_t address, std::uint8_t value) {
    memory->setMemory(static_cast<int>(address), value);
}

std::uint8_t Core::fetch() {
    std::uint8_t data = read(static_cast<std::uint16_t>(pc.getValue()));
    pc.setValue(pc.getValue() + 1);
    return data;
}

std::uint16_t Core::fetchWord() {
    std::uint16_t low = fetch();
    std::uint16_t high = fetch();
    return static_cast<std::uint16_t>((high << 8) | low);
}

void Core::setStatusFlag(StatusFlag flag, bool value) {
    if (value) {
        p.setBit(static_cast<int>(flag));
    } else {
        p.clearBit(static_cast<int>(flag));
    }
}

bool Core::getStatusFlag(StatusFlag flag) const {
    return p.getBit(static_cast<int>(flag));
}