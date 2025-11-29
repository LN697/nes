#pragma once

#include <cstdint>
#include <vector>

class PPU {
public:
    PPU();
    ~PPU();

    uint8_t cpuRead(uint16_t address);
    void cpuWrite(uint16_t address, uint8_t data);
    void step(int cycles = 1);

    bool nmiOccurred = false;

private:
    uint8_t status;        // $2002
    uint8_t control;       // $2000
    uint8_t mask;          // $2001
    
    int cycle;
    int scanline;
    uint8_t address_latch = 0;
    uint8_t data_buffer = 0;
};