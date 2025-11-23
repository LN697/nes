#include "core.hpp"

Core::Core(Memory* memory) : CoreBase(memory) {
    core_phase = Phase::STANDBY;

    a  = new Register(RegType::MR, new std::string("Accumulator"));
    x  = new Register(RegType::IR, new std::string("X index"));
    y  = new Register(RegType::IR, new std::string("Y index"));
    s  = new Register(RegType::IR, new std::string("Stack pointer"));
    pc = new Register(RegType::PC, new std::string("Program Counter"));
    p = new Register(RegType::SR, new std::string("Processor flags"));

    log("CORE", "Core initialized in " + std::to_string(static_cast<int>(core_phase)) + " phase.");
}

Core::~Core() = default;

void Core::step() {
    log("CORE", "DEBUGGING CORE");
}