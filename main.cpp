#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <iomanip>
#include <csignal>
#include <thread>
#include <chrono>

#include "bus.hpp"
#include "core.hpp"
#include "core_policies.hpp"
#include "policies_map.hpp"
#include "logger.hpp"
#include "renderer.hpp"

volatile std::sig_atomic_t g_signal_received = 0;

void signal_handler(int signal) {
    g_signal_received = signal;
}

int main(int argc, char** argv) {
    std::string rom_path;
    if (argc > 1) {
        rom_path = argv[1];
    } else {
        std::cerr << "Usage: " << argv[0] << " <rom_file>\n";
        return 1;
    }
    
    Bus bus;
    Renderer renderer;

    if (!renderer.init("NES Emulator - Pattern Tables", 256, 240, 3)) {
        std::cerr << "Failed to initialize Renderer/SDL.\n";
        return 1;
    }
    
    // 2. Load ROM
    std::ifstream file(rom_path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open ROM file.\n";
        return 1;
    }
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<char> buffer(size);
    
    if (file.read(buffer.data(), size)) {
        // iNES Parsing
        bool is_ines = (size >= 16 && buffer[0] == 'N' && buffer[1] == 'E' && buffer[2] == 'S' && buffer[3] == 0x1A);
        
        if (is_ines) {
            int prg_chunks = buffer[4];
            int chr_chunks = buffer[5]; // Byte 5 is CHR Size (8KB units)
            int prg_size = prg_chunks * 16384;
            int chr_size = chr_chunks * 8192; // 8KB per chunk

            int offset = 16 + ((buffer[6] & 0x04) ? 512 : 0);
            // Byte 6, Bit 0: Mirroring (0 = Horizontal, 1 = Vertical)
            bool vertical_mirroring = (buffer[6] & 0x01) != 0;
            
            // Set Mirroring Mode
            bus.ppu.setMirroring(vertical_mirroring ? PPU::Mirroring::VERTICAL : PPU::Mirroring::HORIZONTAL);
            
            log("Loader", std::string("Mirroring: ") + (vertical_mirroring ? "Vertical" : "Horizontal"));
            std::cout << "[Loader] iNES Header: " << prg_chunks << " PRG, " << chr_chunks << " CHR.\n";

            // 1. Load PRG-ROM (Code)
            std::vector<uint8_t> prgData(prg_size);
            for(int i=0; i<prg_size; i++) {
                if(offset + i < size) prgData[i] = buffer[offset + i];
            }
            bus.loadROM(prgData, (prg_chunks == 1));

            // 2. Load CHR-ROM (Graphics)
            // Offset moves past the PRG data
            int chr_offset = offset + prg_size;
            std::vector<uint8_t> chrData;
            
            if (chr_size > 0) {
                chrData.resize(chr_size);
                for(int i=0; i<chr_size; i++) {
                    if(chr_offset + i < size) chrData[i] = buffer[chr_offset + i];
                }
            } else {
                // If CHR chunks = 0, the game uses CHR-RAM (Writable).
                // Create 8KB of empty writable memory.
                std::cout << "[Loader] CHR-RAM detected (0 chunks).\n";
                chrData.resize(8192);
            }
            
            // Send to PPU
            bus.ppu.setCHR(chrData);

        } else {
            log("Loader", "Loading Raw Binary.");
            std::vector<uint8_t> rawData(size);
            for(int i=0; i<size; i++) rawData[i] = buffer[i];
            bus.loadROM(rawData, false);
        }
    }
    
    Core core(&bus);
    
    init_instr_table(core);

    core.init(); // triggers Reset Vector fetch

    std::signal(SIGINT, signal_handler);
    log("System", "Engine Started.");

    try {
        // Frame Timing (NTSC)
        using namespace std::chrono;
        const nanoseconds frame_duration(16666667);
        auto frame_start_time = high_resolution_clock::now();

        const int CYCLES_PER_FRAME = 29780;
        int cycles_this_frame = 0;

        while (g_signal_received == 0) {
            
            // --- BATCH EXECUTION LOOP ---
            while (cycles_this_frame < CYCLES_PER_FRAME) {
                core.step();
                
                // PPU runs 3x faster than CPU
                int cpu_cycles = core.last_cycles + bus.dma_cycles;
                bus.dma_cycles = 0;
                
                bus.ppu.step(3 * cpu_cycles);
                
                cycles_this_frame += cpu_cycles;

                if (g_signal_received != 0) break;
            }
            
            cycles_this_frame -= CYCLES_PER_FRAME;

            // --- RENDER & INPUT ---
            if (!renderer.handleEvents()) {
                break;
            }
            bus.input.update(); // Poll input state once per frame
            renderer.draw(bus);

            // Sleep until the next frame is due
            std::this_thread::sleep_until(frame_start_time + frame_duration);
            frame_start_time = high_resolution_clock::now();
        }
    } catch (const std::exception& e) {
        std::cerr << "\nCRASH: Exception caught in main loop: " << e.what() << "\n";
        std::cerr << "Last PC: " << std::hex << core.pc.getValue() << std::dec << "\n";
    }

    // Cleanup SDL is handled by Renderer destructor
    return 0;
}