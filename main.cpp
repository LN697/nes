#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <csignal>
#include <memory>

#include "bus.hpp"
#include "core.hpp"
#include "renderer.hpp"
#include "logger.hpp"
#include "policies_map.hpp"
#include "cartridge.hpp"

volatile std::sig_atomic_t g_signal_received = 0;
void signal_handler(int signal) { g_signal_received = signal; }

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <rom_file>\n";
        return 1;
    }

    // 1. Initialize Systems
    Bus bus;
    Renderer renderer;
    if (!renderer.init("NES Emulator", 256, 240, 3)) return 1;

    // 2. Load Cartridge
    auto cart = std::make_shared<Cartridge>(argv[1]);
    if (!cart->ImageValid()) {
        std::cerr << "Failed to load ROM: " << argv[1] << "\n";
        return 1;
    }

    // 3. Connect Cartridge to Bus
    bus.insertCartridge(cart);

    // 4. Initialize CPU
    Core core(&bus);
    init_instr_table(core);
    bus.reset();
    core.init();
    
    std::signal(SIGINT, signal_handler);
    
    // --- TIMING LOOP ---
    using clock = std::chrono::high_resolution_clock;
    auto frame_start = clock::now();
    auto frame_end = clock::now();
    auto frame_duration = frame_end - frame_start;
    const std::chrono::nanoseconds target_frame_duration(16666667); // 60 FPS

    bool frame_complete = false;
    int ppu_cycles = 0;
    int cpu_cycles = 0;

    while (g_signal_received == 0) {
        frame_start = clock::now();

        bus.input.update();
        
        frame_complete = false;
        while (!frame_complete) {
            // 1. Run CPU (returns 2-7 cycles)
            core.step();
            
            // 2. Sync other components to CPU time
            cpu_cycles = core.last_cycles + bus.dma_cycles;
            bus.dma_cycles = 0; // consumed
            
            bus.apu.step(cpu_cycles);

            // 3. PPU runs at 3x CPU speed
            ppu_cycles = cpu_cycles * 3;
            frame_complete = bus.ppu.step(ppu_cycles);
        }

        if (!renderer.handleEvents()) break;
        renderer.draw(bus.ppu.getScreen());

        // Frame Limiter
        frame_end = clock::now();
        frame_duration = frame_end - frame_start;
        if (frame_duration < target_frame_duration) {
            std::this_thread::sleep_for(target_frame_duration - frame_duration);
        }
    }

    return 0;
}