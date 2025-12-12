#include "mapper.hpp"

Mapper::Mapper(uint8_t prgBanks, uint8_t chrBanks) : nPRGBanks(prgBanks), nCHRBanks(chrBanks) {}
Mapper::~Mapper() = default;

void Mapper::reset() {}

MirrorMode Mapper::getMirroringMode() {
    return MirrorMode::HARDWARE;
}

bool Mapper::getIRQ() {
    return false;
}

// =============================================================
// MAPPER 000 (NROM)
// =============================================================

Mapper_000::Mapper_000(uint8_t prgBanks, uint8_t chrBanks) : Mapper(prgBanks, chrBanks) {}
Mapper_000::~Mapper_000() = default;

void Mapper_000::reset() {
    // NROM has no internal state to reset
}

bool Mapper_000::cpuMapRead(uint16_t addr, uint32_t &mapped_addr, uint8_t &data) {
    if (addr >= 0x8000 && addr <= 0xFFFF) {
        mapped_addr = addr & (nPRGBanks > 1 ? 0x7FFF : 0x3FFF);
        return true;
    }
    return false;
}

bool Mapper_000::cpuMapWrite(uint16_t addr, uint32_t &mapped_addr, uint8_t data) {
    if (addr >= 0x8000 && addr <= 0xFFFF) {
        return true; 
    }
    return false;
}

bool Mapper_000::ppuMapRead(uint16_t addr, uint32_t &mapped_addr) {
    if (addr >= 0x0000 && addr <= 0x1FFF) {
        mapped_addr = addr;
        return true;
    }
    return false;
}

bool Mapper_000::ppuMapWrite(uint16_t addr, uint32_t &mapped_addr) {
    if (addr >= 0x0000 && addr <= 0x1FFF) {
        if (nCHRBanks == 0) {
            mapped_addr = addr;
            return true;
        }
    }
    return false;
}