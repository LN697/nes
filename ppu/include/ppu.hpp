#pragma once

#include <cstdint>
#include <vector>
#include <array>

class PPU {
public:
    PPU();
    ~PPU();

    // CPU Interface
    uint8_t cpuRead(uint16_t address);
    void cpuWrite(uint16_t address, uint8_t data);
    
    // System Timing
    void step(int cycles);
    
    // Interrupt Signal
    bool nmiOccurred = false;

    // --- Helpers ---
    uint8_t ppuRead(uint16_t address);
    void ppuWrite(uint16_t address, uint8_t data);
    void setCHR(const std::vector<uint8_t>& rom);
    uint8_t getControl();

private:
    // --- Memory ---
    // 2KB of Nametable VRAM (CIRAM). 
    // In a real NES, mapping logic determines how this 2KB is mirrored into the 4KB address space.
    std::array<uint8_t, 2048> tblName;
    std::array<uint8_t, 32> tblPalette;

    std::vector<uint8_t> chrROM;

    // --- CPU Mapped Registers ---
    uint8_t ppuctrl = 0;   // $2000
    uint8_t ppumask = 0;   // $2001
    uint8_t ppustatus = 0; // $2002
    uint8_t oamaddr = 0;   // $2003
    
    // --- Internal "Loopy" Registers ---
    uint16_t v_ram_addr = 0x0000;
    uint16_t t_ram_addr = 0x0000;
    
    uint8_t fine_x = 0x00;
    
    bool address_latch = false; 

    // --- Data Buffer ---
    uint8_t ppu_data_buffer = 0x00; // For the $2007 read delay

    // --- Timing State ---
    int16_t cycle = 0;
    int16_t scanline = 0; // 0-261
};