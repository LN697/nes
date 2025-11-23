#pragma once

#include "logger.hpp"
#include "common.hpp"

#include "memory.hpp"

enum class Phase { STANDBY, FETCH, OPR_FETCH, READ, OPERATION, WRITE, INTERRUPT };

class CoreBase {
    protected:
        Phase core_phase;
        Memory* memory;

    public:
        CoreBase(Memory* memory);
        virtual ~CoreBase();

        virtual void step();
};