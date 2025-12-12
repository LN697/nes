#pragma once

#include <cstdint>
#include <vector>
#include <array>
#include <memory>
#include "ppu.hpp"
#include "input.hpp"
#include "audio.hpp"
#include "cartridge.hpp"

class Bus {
    public:
        Bus();
        ~Bus();

        // Devices
        PPU ppu;
        Input input;
        APU apu;
        
        // Cartridge Interface
        std::shared_ptr<Cartridge> cart;
        void insertCartridge(const std::shared_ptr<Cartridge>& cartridge);
        
        // System State
        int dma_cycles = 0;
        bool testMode = false;
        std::vector<uint8_t> testRam;

        // CPU Bus Interface
        void write(uint16_t address, uint8_t data);
        uint8_t read(uint16_t address);
        
        // Utilities
        void setTestMode(bool enabled);
        bool getIRQ() const; 
        void reset();

    private:
        std::array<uint8_t, 2048> cpuRam;
};