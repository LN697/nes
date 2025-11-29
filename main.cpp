#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <iomanip>
#include <csignal>
#include <thread>
#include <chrono>

#include "memory.hpp"
#include "core.hpp"
#include "core_policies.hpp"
#include "policies_map.hpp"
#include "logger.hpp"

// Global flag for clean exit on Ctrl+C
volatile std::sig_atomic_t g_signal_received = 0;

void signal_handler(int signal) {
    g_signal_received = signal;
}

// ============================================================================
// MAIN ENTRY POINT
// ============================================================================
int main(int argc, char** argv) {
    std::string rom_path;
    if (argc > 1) {
        rom_path = argv[1];
    } else {
        std::cerr << "Usage: " << argv[0] << " <rom_file>\n";
        return 1;
    }

    // Initialize Memory (64KB)
    Memory mem(64 * 1024);

    // Load ROM
    std::ifstream file(rom_path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open ROM file: " << rom_path << "\n";
        return 1;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    if (size > 65536) {
        std::cerr << "Warning: File size (" << size << ") larger than 64KB memory. Truncating.\n";
    }

    std::vector<char> buffer(size);
    if (file.read(buffer.data(), size)) {
        // ROM Loading Logic
        // 1. Check for iNES Header ("NES" followed by 0x1A)
        bool is_ines = false;
        if (size >= 16 && buffer[0] == 'N' && buffer[1] == 'E' && buffer[2] == 'S' && buffer[3] == 0x1A) {
            is_ines = true;
        }

        if (is_ines) {
            // iNES Loader (Basic NROM support)
            // PRG ROM size in 16KB units is at offset 4
            int prg_chunks = buffer[4];
            int prg_size = prg_chunks * 16384;
            
            std::cout << "[Loader] Detected iNES Header. PRG-ROM Size: " << prg_size << " bytes.\n";

            // Load PRG-ROM into memory. 
            // NROM-128 (1 chunk): Mapped to $8000 and mirrored at $C000.
            // NROM-256 (2 chunks): Mapped to $8000.
            int load_addr = 0x8000;
            
            // iNES header is 16 bytes. Trainer is 512 bytes if bit 2 of byte 6 is set.
            int offset = 16;
            if (buffer[6] & 0x04) {
                offset += 512;
            }

            // Safety check
            if (offset + prg_size > size) {
                std::cerr << "Error: Unexpected end of file reading PRG-ROM.\n";
                return 1;
            }

            for (int i = 0; i < prg_size; ++i) {
                if (load_addr + i < 65536) {
                    mem.setMemory(load_addr + i, static_cast<uint8_t>(buffer[offset + i]));
                }
            }

            // Mirroring for NROM-128
            if (prg_chunks == 1) {
                std::cout << "[Loader] Mirroring 16KB PRG-ROM from $8000 to $C000.\n";
                for (int i = 0; i < 16384; ++i) {
                    mem.setMemory(0xC000 + i, mem.getMemory(0x8000 + i));
                }
            }

        } else {
            // Raw Binary Loader
            // If file is exactly 64KB, load at 0.
            // If file is small (e.g. 4KB test rom), usage convention varies.
            // For safety in "whole opcodes" context, usually implies a flat 64k dump.
            std::cout << "[Loader] Loading raw binary at $0000.\n";
            for (int i = 0; i < size && i < 65536; ++i) {
                mem.setMemory(i, static_cast<uint8_t>(buffer[i]));
            }
        }
    } else {
        std::cerr << "Error: Failed to read file.\n";
        return 1;
    }

    // Initialize Core
    Core core(&mem);
    
    // Initialize Instruction Table
    init_instr_table(core);

    // Boot CPU
    core.run(); // sets PC from vector

    // Setup Signal Handler for clean exit
    std::signal(SIGINT, signal_handler);

    // Main Loop
    std::cout << "[Core] Starting execution at PC: $" << std::hex << core.pc.getValue() << std::dec << "\n";
    
    auto start_time = std::chrono::high_resolution_clock::now();
    uint64_t cycles = 0;

    // Use a simplified run loop
    while (g_signal_received == 0) {
        // Execute one instruction
        
        // --- CUSTOM STEP LOGIC FOR MAIN ---
        // We inline a simplified step() here to allow custom logging if needed
        // and to avoid the "decimal opcode" logging confusion from Core::step
        
        uint8_t opcode = core.fetch();
        
        // Optional: Hex logging (Uncomment for debugging)
        std::cout << "PC:" << std::hex << (core.pc.getValue()-1) << " Op:" << (int)opcode << std::dec << "\n";

        auto handler = core.instr_table[opcode];
        if (handler) {
            handler(core);
        } else {
            std::cerr << "Error: Unimplemented opcode 0x" << std::hex << (int)opcode << std::dec << "\n";
            g_signal_received = 1;
        }
        
        cycles++;
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end_time - start_time;
    
    std::cout << "\n[Core] Stopped. Executed " << cycles << " instructions in " << elapsed.count() << " seconds.\n";
    std::cout << "[Core] Speed: " << (cycles / elapsed.count()) / 1000000.0 << " MHz (approx instructions)\n";

    return 0;
}