#pragma once

#include <cstdint>
#include <vector>
#include <array>
#include <memory>
#include "cartridge.hpp"

class PPU {
    public:
        PPU();
        ~PPU();

        enum class Mirroring {
            HORIZONTAL,
            VERTICAL,
            ONESCREEN_LO,
            ONESCREEN_HI
        };

        // Connectivity
        void connectCartridge(const std::shared_ptr<Cartridge>& cartridge);
        void reset();

        // CPU Interface
        uint8_t cpuRead(uint16_t address);
        void cpuWrite(uint16_t address, uint8_t data);
        
        // System Timing
        bool step(int cycles);
        
        // Interface for Renderer
        const std::vector<uint32_t>& getScreen() const;

        // Interrupt Signal
        bool nmiOccurred = false;

        // Helpers for Bus/DMA
        const std::array<uint8_t, 256>& getOAM() const;
        void startOAMDMA(const std::array<uint8_t, 256>& data);

    private:
        std::shared_ptr<Cartridge> cart;

        bool suppress_vbl = false;
        uint32_t applyGrayscale(uint32_t color);
        uint32_t applyEmphasis(uint32_t color);
        
        // --- Memory ---
        std::array<uint8_t, 2048> tblName;
        std::array<uint8_t, 32> tblPalette;
        // chrROM is removed; access goes through 'cart'
        std::array<uint8_t, 256> oamData;

        // --- Screen Buffer ---
        std::vector<uint32_t> pixels;
        static const std::array<uint32_t, 64> systemPalette;

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
        uint16_t bg_shifter_pattern_lo = 0x0000;
        uint16_t bg_shifter_pattern_hi = 0x0000;
        uint16_t bg_shifter_attrib_lo = 0x0000;
        uint16_t bg_shifter_attrib_hi = 0x0000;

        // --- Latches (Next Tile Data) ---
        uint8_t bg_next_tile_id = 0x00;
        uint8_t bg_next_tile_attrib = 0x00;
        uint8_t bg_next_tile_lsb = 0x00;
        uint8_t bg_next_tile_msb = 0x00;

        // --- Internal Variables ---
        uint8_t bg_pixel = 0x00;
        uint8_t bg_palette = 0x00;
        bool bg_opaque = false;
        uint8_t sp_pixel = 0x00;
        uint8_t sp_palette = 0x00;
        bool sp_priority = false; 
        bool sp_0_rendered = false;

        // --- Sprite Rendering State ---
        struct ObjectAttrEntry {
            uint8_t y;
            uint8_t id;
            uint8_t attribute;
            uint8_t x;
            bool isZero;
        };
        
        std::vector<ObjectAttrEntry> spriteScanline; 
        uint8_t sprite_count = 0;
        bool bSpriteZeroHitPossible = false;
        bool bSpriteZeroBeingRendered = false;

        // --- Timing State ---
        int16_t cycle = 0;
        int16_t scanline = 0; 
        uint64_t frame_count = 0;
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
        void evaluateSprites(); 
};