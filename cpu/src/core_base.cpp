#include "core_base.hpp"

CoreBase::CoreBase(Memory* memory) : core_phase(Phase::STANDBY), memory(memory) {}

CoreBase::~CoreBase() = default;

void CoreBase::step() {
    // Base step does nothing; derived classes should override if needed.
}
