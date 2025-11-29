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

    // 1. Create the System Bus
    Bus bus;

    // 2. Load ROM
    std::ifstream file(rom_path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open ROM.\n";
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
            int prg_size = prg_chunks * 16384;
            int offset = 16 + ((buffer[6] & 0x04) ? 512 : 0); // Skip header + trainer
            
            std::cout << "[Loader] Loading iNES ROM. PRG Size: " << prg_size << " bytes.\n";
            
            std::vector<uint8_t> prgData;
            prgData.resize(prg_size);
            for(int i=0; i<prg_size; i++) {
                if(offset + i < size) prgData[i] = buffer[offset + i];
            }
            
            // FIX: Pass the mirroring flag (True if 16KB NROM-128, False if 32KB NROM-256)
            bool mirror = (prg_chunks == 1);
            bus.loadROM(prgData, mirror);

        } else {
            std::cout << "[Loader] Loading Raw Binary.\n";
            std::vector<uint8_t> rawData(size);
            for(int i=0; i<size; i++) rawData[i] = buffer[i];
            
            // For raw binaries, assume 32KB mapping or flat load depending on size
            // Passing false for mirror as default
            bus.loadROM(rawData, false);
        }
    }

    // 3. Create Core attached to Bus
    Core core(&bus);
    
    // 4. Initialize Tables & Reset
    init_instr_table(core);
    core.run(); // Triggers Reset Vector fetch

    std::signal(SIGINT, signal_handler);
    
    std::cout << "[System] Engine Started.\n";

    // 5. Main Execution Loop
    while (g_signal_received == 0) {
        core.step(); 
        int cycles_spent = 3; 
        bus.ppu.step(cycles_spent * 3);
    }

    std::cout << "\n[System] Stopped.\n";
    return 0;
}