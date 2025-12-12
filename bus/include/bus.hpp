#pragma once

#include <cstdint>
#include <vector>
#include <array>
#include <algorithm>
#include "ppu.hpp"
#include "input.hpp"
#include "audio.hpp"

class Bus {
    public:
        Bus();
        ~Bus();

        PPU ppu;
        Input input;
        APU apu; // Added APU instance

        int dma_cycles = 0;
        
        bool testMode = false;

        std::vector<uint8_t> testRam;

        void setTestMode(bool enabled);
        uint8_t read(uint16_t address);
        void write(uint16_t address, uint8_t data);
        void loadROM(const std::vector<uint8_t>& prg, bool mirror_upper);
        
        bool getIRQ() const; // For Core to poll

    private:
        std::array<uint8_t, 2048> cpuRam;
        std::vector<uint8_t> cartridgeROM;

        uint8_t readCartridge(uint16_t address);
};