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

    #ifdef UNIT_TEST
        // Test-only helpers
        void test_incrementScrollX();
        void test_incrementScrollY();
        void test_transferAddressX();
        void test_transferAddressY();
        uint16_t getVramAddr() const { return v_ram_addr; }
        void setVramAddr(uint16_t v) { v_ram_addr = v; }
        uint16_t getTRamAddr() const { return t_ram_addr; }
        int getScanline() const { return scanline; }
        int getCycle() const { return cycle; }
        uint8_t getBgNextAttrib() const { return bg_next_tile_attrib; }
        uint8_t getFineX() const { return fine_x; }
        uint16_t getBgShifterPatternLo() const { return bg_shifter_pattern_lo; }
        uint16_t getBgShifterPatternHi() const { return bg_shifter_pattern_hi; }
        uint16_t getBgShifterAttribLo() const { return bg_shifter_attrib_lo; }
        uint16_t getBgShifterAttribHi() const { return bg_shifter_attrib_hi; }
        uint8_t getBgNextTileLsb() const { return bg_next_tile_lsb; }
        uint8_t getBgNextTileMsb() const { return bg_next_tile_msb; }
        const std::vector<uint8_t>& getDebugBgPixels() const { return debug_bg_pixel; }
        const std::vector<uint8_t>& getDebugBgPalettes() const { return debug_bg_palette; }
        const std::vector<uint8_t>& getDebugSpPixels() const { return debug_sp_pixel; }
        const std::vector<uint8_t>& getDebugSpPalettes() const { return debug_sp_palette; }
        const std::vector<uint32_t>& getDebugWriteCycles() const { return debug_write_cycle; }
        const std::vector<uint16_t>& getDebugWriteScanlines() const { return debug_write_scanline; }
        const std::vector<uint16_t>& getDebugWriteVram() const { return debug_write_vram; }
    #endif

    private:
        // --- Memory ---
        std::array<uint8_t, 2048> tblName;
        std::array<uint8_t, 32> tblPalette;
        std::vector<uint8_t> chrROM;
        std::array<uint8_t, 256> oamData;

        // --- Screen Buffer ---
        std::vector<uint32_t> pixels;
        static const std::array<uint32_t, 64> systemPalette;
        #ifdef UNIT_TEST
        std::vector<uint8_t> debug_bg_pixel;   
        std::vector<uint8_t> debug_bg_palette; 
        std::vector<uint8_t> debug_sp_pixel;
        std::vector<uint8_t> debug_sp_palette;
        #endif

        // --- Configuration ---
        Mirroring mirroring = Mirroring::VERTICAL;

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
        
        #ifdef UNIT_TEST
        uint64_t master_cycle = 0; 
        std::vector<uint32_t> debug_write_cycle; 
        std::vector<uint16_t> debug_write_scanline; 
        std::vector<uint16_t> debug_write_vram; 
        #endif

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
        
        // NEW: Sprite Evaluation for the next line
        void evaluateSprites(); 
};