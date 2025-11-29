#include "logger.hpp"
#include "ppu.hpp"

PPU::PPU() {
    // Initialize registers
    status = 0x00;
    cycle = 0;
    scanline = 0;
}

PPU::~PPU() = default;

// Connect to the bus: CPU reads from $2000-$2007
uint8_t PPU::cpuRead(uint16_t address) {
    uint8_t data = 0x00;
    switch (address) {
        case 0x0002: // Status Register ($2002)
            // Reading status clears the VBlank flag (Bit 7) and the Address Latch
            data = (status & 0xE0) | (data_buffer & 0x1F); // Noise from buffer
            status &= ~0x80; // Clear VBlank flag
            address_latch = 0;
            break;

        default:
            break;
    }
    return data;
}

void PPU::cpuWrite(uint16_t address, uint8_t data) {
    switch (address) {
        case 0x0000: // Control
            // NMI generation logic would go here
            break;
        default:
            break;
    }
}

// Step the PPU by 1 cycle (3 PPU cycles = 1 CPU cycle on NTSC)
void PPU::step(int cycles) {
    cycle += cycles;
    
    // Basic NTSC Timing: 341 cycles per scanline, 262 scanlines total
    if (cycle >= 341) {
        cycle -= 341;
        scanline++;

        if (scanline >= 261) {
            scanline = -1; // Pre-render scanline
        }
    }

    // Set VBlank Flag at Scanline 241, Cycle 1
    if (scanline == 241 && cycle == 1) {
        status |= 0x80; // Set Bit 7 (VBlank)
        // Trigger NMI here if enabled in Control register
    }
    
    // Clear VBlank Flag at Pre-render line
    if (scanline == -1 && cycle == 1) {
        status &= ~0x80;
    }
}