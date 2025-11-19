#pragma once

#include "logger.hpp"

#include "register.hpp"
#include "memory.hpp"

enum class Phase { FETCH, OPR_FETCH, READ, OPERATION, WRITE, INTERRUPT };

class CoreBase {
    private:
        Phase core_phase;
        Memory* memory;

    public:
        

};