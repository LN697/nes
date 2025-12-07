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


    while (g_signal_received == 0) {        
        // Emulation Loop
        frame_complete = false;
        while (!frame_complete) {
            core.step();
            // Run PPU 3 times per CPU cycle
            int cycles = (core.last_cycles + bus.dma_cycles) * 3;
            bus.dma_cycles = 0;
            
            frame_complete = bus.ppu.step(cycles);
        }

        // Render
        if (!renderer.handleEvents()) break;
        bus.input.update();
        renderer.draw(bus.ppu.getScreen());
    }

    return 0;
}