#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <csignal>
#include <cstring>

#include "bus.hpp"
#include "core.hpp"
#include "renderer.hpp"
#include "logger.hpp"
#include "policies_map.hpp"

volatile std::sig_atomic_t g_signal_received = 0;
void signal_handler(int signal) { g_signal_received = signal; }

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <rom_file>\n";
        return 1;
    }

    Bus bus;
    Renderer renderer;
    if (!renderer.init("NES Emulator", 256, 240, 3)) return 1;

    std::ifstream file(argv[1], std::ios::binary | std::ios::ate);
    if (!file.is_open()) return 1;
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<char> buffer(size);
    if (!file.read(buffer.data(), size)) return 1;

    // iNES Header Parser
    if (buffer[0] == 'N' && buffer[1] == 'E' && buffer[2] == 'S' && buffer[3] == 0x1A) {
        int prg_chunks = buffer[4];
        int chr_chunks = buffer[5];
        bool vertical = (buffer[6] & 0x01);
        
        bus.ppu.setMirroring(vertical ? PPU::Mirroring::VERTICAL : PPU::Mirroring::HORIZONTAL);
        
        std::vector<uint8_t> prg(prg_chunks * 16384);

        int offset = 16 + ((buffer[6] & 0x04) ? 512 : 0);
        std::memcpy(prg.data(), &buffer[offset], prg.size());
        bus.loadROM(prg, prg_chunks == 1);

        int chr_offset = offset + prg.size();
        std::vector<uint8_t> chr(chr_chunks > 0 ? chr_chunks * 8192 : 8192);
        if (chr_chunks > 0) std::memcpy(chr.data(), &buffer[chr_offset], chr.size());
        bus.ppu.setCHR(chr);
    }

    Core core(&bus);
    init_instr_table(core);
    core.init();
    
    std::signal(SIGINT, signal_handler);
    bool frame_complete;
    int ppu_cycles;
    int cpu_cycles;

    // --- TIMING SETUP ---
    using clock = std::chrono::high_resolution_clock;
    auto frame_start = clock::now();
    auto frame_end = clock::now();
    auto frame_duration = frame_end - frame_start;

    // Target 60 FPS (approx 16.67ms per frame)
    const std::chrono::nanoseconds target_frame_duration(16666667); 

    while (g_signal_received == 0) {
        frame_start = clock::now();

        bus.input.update();
        
        // --- INTERLEAVED EXECUTION LOOP ---
        // Instead of running CPU for a whole frame, we run it for ONE instruction.
        // Then we run the PPU for 3x that amount. This keeps them in sync.
        frame_complete = false;
        while (!frame_complete) {
            // 1. Run CPU (returns after 1 instruction, e.g., 2-7 cycles)
            core.step();
            
            // 2. Account for CPU cycles + any DMA cycles that occurred
            cpu_cycles = core.last_cycles + bus.dma_cycles;
            
            // 3. Clock APU (Runs at CPU speed)
            bus.apu.step(cpu_cycles);

            // 4. Clock PPU (Runs at 3x CPU speed)
            ppu_cycles = cpu_cycles * 3;
            
            // Reset DMA counter (it was consumed by this step)
            bus.dma_cycles = 0;
            
            // 5. Run PPU
            // returns true only when VBlank starts (Scanline 241, Cycle 1)
            frame_complete = bus.ppu.step(ppu_cycles);
        }

        // Render the frame that was just completed by the PPU
        if (!renderer.handleEvents()) break;
        renderer.draw(bus.ppu.getScreen());

        // --- FRAME LIMITER ---
        frame_end = clock::now();
        frame_duration = frame_end - frame_start;
        
        if (frame_duration < target_frame_duration) {
            std::this_thread::sleep_for(target_frame_duration - frame_duration);
        }
    }

    return 0;
}