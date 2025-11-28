#pragma once

#include <atomic>
#include "logger.hpp"
#include "common.hpp"

#include "memory.hpp"

enum class Phase { STANDBY, FETCH, OPR_FETCH, READ, OPERATION, WRITE, INTERRUPT, ERROR };

class CoreBase {
    private:
        inline static std::atomic<int> core_id_counter = 0;

    protected:
        Phase core_phase;
        Memory* memory;

    public:
        const int core_id;

        CoreBase(Memory* memory);

        virtual ~CoreBase();

        virtual void step(int ops);
        virtual void run();
        virtual void init();
};