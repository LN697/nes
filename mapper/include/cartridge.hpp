#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <memory>
#include "mapper.hpp"

class Cartridge {
public:
    Cartridge(const std::string& sFileName);
    ~Cartridge();

    // Communication with Main Bus
    bool cpuRead(uint16_t addr, uint8_t &data);
    bool cpuWrite(uint16_t addr, uint8_t data);

    // Communication with PPU Bus
    bool ppuRead(uint16_t addr, uint8_t &data);
    bool ppuWrite(uint16_t addr, uint8_t data);
    
    // Utility
    bool ImageValid();
    MirrorMode getMirroring();
    void reset();
    
    // Signals from Mapper
    bool getIRQ();

private:
    std::vector<uint8_t> vPRGMemory;
    std::vector<uint8_t> vCHRMemory;

    uint8_t nMapperID = 0;
    uint8_t nPRGBanks = 0;
    uint8_t nCHRBanks = 0;

    std::shared_ptr<Mapper> pMapper;
    
    bool bImageValid = false;
    MirrorMode hwMirror = MirrorMode::HORIZONTAL;
};