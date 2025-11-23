#include "memory.hpp"
#include "logger.hpp"
#include "core.hpp"
#include "register.hpp"
#include <sstream>

int main() {
    log("MAIN", "Configuring the CPU emulator for NES.");
    Memory mem(64 * KB);
    mem.setMemory(0, 0xFF);
    Core core(&mem);
    core.step();
    
    return 0;
}