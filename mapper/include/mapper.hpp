#pragma once
#include <cstdint>
#include <vector>

enum class MirrorMode {
    HORIZONTAL,
    VERTICAL,
    ONESCREEN_LO,
    ONESCREEN_HI,
    HARDWARE
};

class Mapper {
public:
    Mapper(uint8_t prgBanks, uint8_t chrBanks);
    virtual ~Mapper();

    // Transform CPU bus address to PRG ROM offset
    virtual bool cpuMapRead(uint16_t addr, uint32_t &mapped_addr, uint8_t &data) = 0;
    virtual bool cpuMapWrite(uint16_t addr, uint32_t &mapped_addr, uint8_t data = 0) = 0;

    // Transform PPU bus address to CHR ROM offset
    virtual bool ppuMapRead(uint16_t addr, uint32_t &mapped_addr) = 0;
    virtual bool ppuMapWrite(uint16_t addr, uint32_t &mapped_addr) = 0;

    virtual void reset();
    virtual MirrorMode getMirroringMode();
    
    // IRQ Interface
    virtual bool getIRQ();

protected:
    uint8_t nPRGBanks = 0;
    uint8_t nCHRBanks = 0;
};

// =============================================================
// MAPPER 000 (NROM)
// =============================================================
class Mapper_000 : public Mapper {
public:
    Mapper_000(uint8_t prgBanks, uint8_t chrBanks);
    ~Mapper_000() override;

    bool cpuMapRead(uint16_t addr, uint32_t &mapped_addr, uint8_t &data) override;
    bool cpuMapWrite(uint16_t addr, uint32_t &mapped_addr, uint8_t data = 0) override;
    bool ppuMapRead(uint16_t addr, uint32_t &mapped_addr) override;
    bool ppuMapWrite(uint16_t addr, uint32_t &mapped_addr) override;
    void reset() override;
};