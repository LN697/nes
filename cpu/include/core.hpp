#pragma once

#include "core_base.hpp"
#include "register.hpp"

class Core : public CoreBase {
    private:
        Register* a;
        Register* x;
        Register* y;
        Register* s;
        Register* pc;
        Register* p;
    public:
        Core(Memory* memory);
        ~Core() override;

        void step() override;
};