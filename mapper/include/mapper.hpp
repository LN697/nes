#pragma once
#include <cstdint>
#include <vector>
#include <array>

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
    virtual void clearIRQ();
    virtual void scanline(); // Optional helper for scanline counters

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
    bool cpuMapRead(uint16_t addr, uint32_t &mapped_addr, uint8_t &data) override;
    bool cpuMapWrite(uint16_t addr, uint32_t &mapped_addr, uint8_t data = 0) override;
    bool ppuMapRead(uint16_t addr, uint32_t &mapped_addr) override;
    bool ppuMapWrite(uint16_t addr, uint32_t &mapped_addr) override;
    void reset() override;
};

// =============================================================
// MAPPER 001 (MMC1) - Zelda, Metroid
// =============================================================
class Mapper_001 : public Mapper {
public:
    Mapper_001(uint8_t prgBanks, uint8_t chrBanks);
    bool cpuMapRead(uint16_t addr, uint32_t &mapped_addr, uint8_t &data) override;
    bool cpuMapWrite(uint16_t addr, uint32_t &mapped_addr, uint8_t data = 0) override;
    bool ppuMapRead(uint16_t addr, uint32_t &mapped_addr) override;
    bool ppuMapWrite(uint16_t addr, uint32_t &mapped_addr) override;
    void reset() override;
    MirrorMode getMirroringMode() override;

private:
    uint8_t nLoadRegister = 0x00;
    uint8_t nLoadRegisterCount = 0x00;
    uint8_t nControlRegister = 0x00;
    uint8_t nCHRBankSelect4Lo = 0x00;
    uint8_t nCHRBankSelect4Hi = 0x00;
    uint8_t nPRGBankSelect16Lo = 0x00;
    uint8_t nPRGBankSelect16Hi = 0x00; // Not strictly needed for std MMC1 but good for consistency
    uint8_t nPRGBankSelect32 = 0x00;

    std::vector<uint8_t> vRAMStatic;
};

// =============================================================
// MAPPER 002 (UNROM) - Castlevania, Contra
// =============================================================
class Mapper_002 : public Mapper {
public:
    Mapper_002(uint8_t prgBanks, uint8_t chrBanks);
    bool cpuMapRead(uint16_t addr, uint32_t &mapped_addr, uint8_t &data) override;
    bool cpuMapWrite(uint16_t addr, uint32_t &mapped_addr, uint8_t data = 0) override;
    bool ppuMapRead(uint16_t addr, uint32_t &mapped_addr) override;
    bool ppuMapWrite(uint16_t addr, uint32_t &mapped_addr) override;
    void reset() override;

private:
    uint8_t nPRGBankSelectLo = 0x00;
};

// =============================================================
// MAPPER 003 (CNROM) - Cybernoid
// =============================================================
class Mapper_003 : public Mapper {
public:
    Mapper_003(uint8_t prgBanks, uint8_t chrBanks);
    bool cpuMapRead(uint16_t addr, uint32_t &mapped_addr, uint8_t &data) override;
    bool cpuMapWrite(uint16_t addr, uint32_t &mapped_addr, uint8_t data = 0) override;
    bool ppuMapRead(uint16_t addr, uint32_t &mapped_addr) override;
    bool ppuMapWrite(uint16_t addr, uint32_t &mapped_addr) override;
    void reset() override;

private:
    uint8_t nCHRBankSelect = 0x00;
};

// =============================================================
// MAPPER 004 (MMC3) - SMB3, Kirby
// =============================================================
class Mapper_004 : public Mapper {
public:
    Mapper_004(uint8_t prgBanks, uint8_t chrBanks);
    bool cpuMapRead(uint16_t addr, uint32_t &mapped_addr, uint8_t &data) override;
    bool cpuMapWrite(uint16_t addr, uint32_t &mapped_addr, uint8_t data = 0) override;
    bool ppuMapRead(uint16_t addr, uint32_t &mapped_addr) override;
    bool ppuMapWrite(uint16_t addr, uint32_t &mapped_addr) override;
    void reset() override;
    MirrorMode getMirroringMode() override;
    bool getIRQ() override;
    void clearIRQ() override;

private:
    uint8_t nTargetRegister = 0x00;
    bool bPRGBankMode = false;
    bool bCHRInversion = false;
    
    MirrorMode mirroring = MirrorMode::HORIZONTAL;
    
    std::array<uint32_t, 8> pRegister;
    std::array<uint32_t, 4> pCHRBank;
    std::array<uint32_t, 4> pPRGBank;

    bool bIRQActive = false;
    bool bIRQEnable = false;
    bool bIRQUpdate = false;
    uint8_t nIRQCounter = 0x00;
    uint8_t nIRQLatch = 0x00;
    uint8_t nIRQReload = 0x00;

    std::vector<uint8_t> vRAMStatic;
};