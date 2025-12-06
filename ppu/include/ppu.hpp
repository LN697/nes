#pragma once

#include <cstdint>
#include <vector>
#include <array>
#include <memory>

class PPU {
public:
    PPU();
    ~PPU();

    enum class Mirroring {
        HORIZONTAL,
        VERTICAL,
        ONE_SCREEN_LOW,
        ONE_SCREEN_HIGH
    };

    // CPU Interface
    uint8_t cpuRead(uint16_t address);
    void cpuWrite(uint16_t address, uint8_t data);

    // Configuration
    void setMirroring(Mirroring mode);
    
    // System Timing
    bool step(int cycles);
    
    // Interface for Renderer
    const std::vector<uint32_t>& getScreen() const;

    // Interrupt Signal
    bool nmiOccurred = false;

    // Helpers
    void setCHR(const std::vector<uint8_t>& rom);
    const std::array<uint8_t, 256>& getOAM() const;
    void startOAMDMA(const std::array<uint8_t, 256>& data);

private:
    // --- Memory ---
    std::array<uint8_t, 2048> tblName;
    std::array<uint8_t, 32> tblPalette;
    std::vector<uint8_t> chrROM;
    std::array<uint8_t, 256> oamData;

    // --- Screen Buffer ---
    std::vector<uint32_t> pixels;
    static const std::array<uint32_t, 64> systemPalette;

    // --- Configuration ---
    Mirroring mirroring = Mirroring::HORIZONTAL;

    // --- Registers ---
    uint8_t ppuctrl = 0;   // $2000
    uint8_t ppumask = 0;   // $2001
    uint8_t ppustatus = 0; // $2002
    uint8_t oamaddr = 0;   // $2003
    
    // --- Internal Registers (Loopy) ---
    uint16_t v_ram_addr = 0x0000;
    uint16_t t_ram_addr = 0x0000;
    uint8_t fine_x = 0x00;
    bool address_latch = false; 
    uint8_t ppu_data_buffer = 0x00; 

    // --- Shift Registers (Background) ---
    // These hold the pattern (bitmap) and palette data for the current and next tile
    uint16_t bg_shifter_pattern_lo = 0x0000;
    uint16_t bg_shifter_pattern_hi = 0x0000;
    uint16_t bg_shifter_attrib_lo = 0x0000;
    uint16_t bg_shifter_attrib_hi = 0x0000;

    // --- Latches (Next Tile Data) ---
    uint8_t bg_next_tile_id = 0x00;
    uint8_t bg_next_tile_attrib = 0x00;
    uint8_t bg_next_tile_lsb = 0x00;
    uint8_t bg_next_tile_msb = 0x00;

    // --- Timing State ---
    int16_t cycle = 0;
    int16_t scanline = 0; 
    bool frame_complete = false;

    // --- Internal Helpers ---
    uint8_t ppuRead(uint16_t address);
    void ppuWrite(uint16_t address, uint8_t data);
    
    uint32_t getColorFromPaletteRam(uint8_t palette, uint8_t pixel);
    
    void incrementScrollX();
    void incrementScrollY();
    void transferAddressX();
    void transferAddressY();
    
    void loadBackgroundShifters();
    void updateShifters();
    
    void renderPixel();
};