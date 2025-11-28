#include "core_base.hpp"

CoreBase::CoreBase(Memory* memory) : core_phase(Phase::STANDBY), memory(memory), core_id(core_id_counter.fetch_add(1)) {}

CoreBase::~CoreBase() = default;

void CoreBase::step(int ops) {
    // Base step does nothing; derived classes should override if needed.
}

void CoreBase::run() {
    // Base run does nothing; derived classes should override if needed.
}
void CoreBase::init() {
    // Base init does nothing; derived classes should override if needed.
}