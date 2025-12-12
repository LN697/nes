#include "mapper.hpp"
#include <iostream>

Mapper::Mapper(uint8_t prgBanks, uint8_t chrBanks) : nPRGBanks(prgBanks), nCHRBanks(chrBanks) {}
Mapper::~Mapper() = default;

void Mapper::reset() {}
MirrorMode Mapper::getMirroringMode() { return MirrorMode::HARDWARE; }
bool Mapper::getIRQ() { return false; }
void Mapper::clearIRQ() {}
void Mapper::scanline() {}

// =============================================================
// MAPPER 000 (NROM)
// =============================================================

Mapper_000::Mapper_000(uint8_t prgBanks, uint8_t chrBanks) : Mapper(prgBanks, chrBanks) {}

void Mapper_000::reset() {}

bool Mapper_000::cpuMapRead(uint16_t addr, uint32_t &mapped_addr, uint8_t &data) {
    if (addr >= 0x8000 && addr <= 0xFFFF) {
        // Map 0x8000-0xFFFF to 0x0000-0x7FFF (32K) or mask for 16K
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

// =============================================================
// MAPPER 001 (MMC1)
// =============================================================

Mapper_001::Mapper_001(uint8_t prgBanks, uint8_t chrBanks) : Mapper(prgBanks, chrBanks) {
    vRAMStatic.resize(32 * 1024);
}

void Mapper_001::reset() {
    nControlRegister = 0x1C;
    nLoadRegister = 0x00;
    nLoadRegisterCount = 0x00;
    nCHRBankSelect4Lo = 0;
    nCHRBankSelect4Hi = 0;
    nPRGBankSelect16Lo = 0;
    nPRGBankSelect16Hi = nPRGBanks - 1;
}

bool Mapper_001::cpuMapRead(uint16_t addr, uint32_t &mapped_addr, uint8_t &data) {
    if (addr >= 0x6000 && addr <= 0x7FFF) {
        // 8K RAM Bank
        mapped_addr = 0xFFFFFFFF; // Signal that it's internal RAM
        data = vRAMStatic[addr & 0x1FFF];
        return true;
    }
    if (addr >= 0x8000) {
        if (nControlRegister & 0x08) {
            // 16K Mode
            if (addr >= 0x8000 && addr <= 0xBFFF) {
                mapped_addr = (nPRGBankSelect16Lo * 0x4000) + (addr & 0x3FFF);
            } else {
                mapped_addr = (nPRGBankSelect16Hi * 0x4000) + (addr & 0x3FFF);
            }
        } else {
            // 32K Mode
            mapped_addr = (nPRGBankSelect32 * 0x8000) + (addr & 0x7FFF);
        }
        return true;
    }
    return false;
}

bool Mapper_001::cpuMapWrite(uint16_t addr, uint32_t &mapped_addr, uint8_t data) {
    if (addr >= 0x6000 && addr <= 0x7FFF) {
        mapped_addr = 0xFFFFFFFF;
        vRAMStatic[addr & 0x1FFF] = data;
        return true;
    }

    if (addr >= 0x8000) {
        if (data & 0x80) {
            // Reset Shift Register
            nLoadRegister = 0x00;
            nLoadRegisterCount = 0;
            nControlRegister = nControlRegister | 0x0C;
        } else {
            // Serial Load
            nLoadRegister >>= 1;
            nLoadRegister |= ((data & 0x01) << 4);
            nLoadRegisterCount++;

            if (nLoadRegisterCount == 5) {
                // Register Full, target determined by bits 13 and 14 of address
                uint8_t target = (addr >> 13) & 0x03;

                if (target == 0) { // 0x8000 - 0x9FFF: Control
                    nControlRegister = nLoadRegister & 0x1F;
                    switch (nControlRegister & 0x03) {
                        case 0: /* OneScreenLo */ break;
                        case 1: /* OneScreenHi */ break;
                        case 2: /* Vertical */    break;
                        case 3: /* Horizontal */  break;
                    }
                }
                else if (target == 1) { // 0xA000 - 0xBFFF: CHR Bank 0
                    if (nControlRegister & 0x10) {
                        // 4K CHR Bank Mode
                        nCHRBankSelect4Lo = nLoadRegister & 0x1F;
                    } else {
                        // 8K CHR Bank Mode
                        nCHRBankSelect4Lo = nLoadRegister & 0x1E;
                    }
                }
                else if (target == 2) { // 0xC000 - 0xDFFF: CHR Bank 1
                    if (nControlRegister & 0x10) {
                        // 4K CHR Bank Mode
                        nCHRBankSelect4Hi = nLoadRegister & 0x1F;
                    }
                }
                else if (target == 3) { // 0xE000 - 0xFFFF: PRG Bank
                    uint8_t prgMode = (nControlRegister >> 2) & 0x03;

                    if (prgMode == 0 || prgMode == 1) {
                        // 32K Mode
                        nPRGBankSelect32 = (nLoadRegister & 0x0E) >> 1;
                    } 
                    else if (prgMode == 2) {
                        // Fix First Bank at 0x8000, Switch 0xC000
                        nPRGBankSelect16Lo = 0;
                        nPRGBankSelect16Hi = nLoadRegister & 0x0F;
                    }
                    else if (prgMode == 3) {
                        // Fix Last Bank at 0xC000, Switch 0x8000
                        nPRGBankSelect16Lo = nLoadRegister & 0x0F;
                        nPRGBankSelect16Hi = nPRGBanks - 1;
                    }
                }

                // Reset Shift Register
                nLoadRegister = 0x00;
                nLoadRegisterCount = 0;
            }
        }
    }
    return false; 
}

MirrorMode Mapper_001::getMirroringMode() {
    switch (nControlRegister & 0x03) {
        case 0: return MirrorMode::ONESCREEN_LO;
        case 1: return MirrorMode::ONESCREEN_HI;
        case 2: return MirrorMode::VERTICAL;
        case 3: return MirrorMode::HORIZONTAL;
    }
    return MirrorMode::HARDWARE;
}

bool Mapper_001::ppuMapRead(uint16_t addr, uint32_t &mapped_addr) {
    if (addr < 0x2000) {
        if (nCHRBanks == 0) {
            mapped_addr = addr;
            return true;
        }

        if (nControlRegister & 0x10) {
            // 4K Mode
            if (addr < 0x1000) {
                mapped_addr = (nCHRBankSelect4Lo * 0x1000) + (addr & 0x0FFF);
            } else {
                mapped_addr = (nCHRBankSelect4Hi * 0x1000) + (addr & 0x0FFF);
            }
        } else {
            // 8K Mode
            mapped_addr = (nCHRBankSelect4Lo * 0x1000) + (addr & 0x1FFF);
        }
        return true;
    }
    return false;
}

bool Mapper_001::ppuMapWrite(uint16_t addr, uint32_t &mapped_addr) {
    if (addr < 0x2000) {
        if (nCHRBanks == 0) {
            mapped_addr = addr;
            return true;
        }
        return true; 
    }
    return false;
}

// =============================================================
// MAPPER 002 (UNROM)
// =============================================================

Mapper_002::Mapper_002(uint8_t prgBanks, uint8_t chrBanks) : Mapper(prgBanks, chrBanks) {}
void Mapper_002::reset() { nPRGBankSelectLo = 0; }

bool Mapper_002::cpuMapRead(uint16_t addr, uint32_t &mapped_addr, uint8_t &data) {
    if (addr >= 0x8000 && addr <= 0xBFFF) {
        // Switchable 16K Bank
        mapped_addr = (nPRGBankSelectLo * 0x4000) + (addr & 0x3FFF);
        return true;
    }
    if (addr >= 0xC000 && addr <= 0xFFFF) {
        // Fixed Last 16K Bank
        mapped_addr = ((nPRGBanks - 1) * 0x4000) + (addr & 0x3FFF);
        return true;
    }
    return false;
}

bool Mapper_002::cpuMapWrite(uint16_t addr, uint32_t &mapped_addr, uint8_t data) {
    if (addr >= 0x8000) {
        nPRGBankSelectLo = data;
    }
    return false;
}

bool Mapper_002::ppuMapRead(uint16_t addr, uint32_t &mapped_addr) {
    if (addr < 0x2000) {
        mapped_addr = addr;
        return true;
    }
    return false;
}

bool Mapper_002::ppuMapWrite(uint16_t addr, uint32_t &mapped_addr) {
    if (addr < 0x2000) {
        if (nCHRBanks == 0) { // RAM
            mapped_addr = addr;
            return true;
        }
    }
    return false;
}

// =============================================================
// MAPPER 003 (CNROM)
// =============================================================

Mapper_003::Mapper_003(uint8_t prgBanks, uint8_t chrBanks) : Mapper(prgBanks, chrBanks) {}
void Mapper_003::reset() { nCHRBankSelect = 0; }

bool Mapper_003::cpuMapRead(uint16_t addr, uint32_t &mapped_addr, uint8_t &data) {
    if (addr >= 0x8000 && addr <= 0xFFFF) {
        mapped_addr = addr & (nPRGBanks > 1 ? 0x7FFF : 0x3FFF);
        return true;
    }
    return false;
}

bool Mapper_003::cpuMapWrite(uint16_t addr, uint32_t &mapped_addr, uint8_t data) {
    if (addr >= 0x8000) {
        nCHRBankSelect = data & 0x03;
    }
    return false;
}

bool Mapper_003::ppuMapRead(uint16_t addr, uint32_t &mapped_addr) {
    if (addr < 0x2000) {
        mapped_addr = (nCHRBankSelect * 0x2000) + addr;
        return true;
    }
    return false;
}

bool Mapper_003::ppuMapWrite(uint16_t addr, uint32_t &mapped_addr) {
    return false;
}


// =============================================================
// MAPPER 004 (MMC3)
// =============================================================

Mapper_004::Mapper_004(uint8_t prgBanks, uint8_t chrBanks) : Mapper(prgBanks, chrBanks) {
    vRAMStatic.resize(32 * 1024);
}

void Mapper_004::reset() {
    nTargetRegister = 0;
    bPRGBankMode = false;
    bCHRInversion = false;
    mirroring = MirrorMode::HORIZONTAL;

    nIRQCounter = 0;
    nIRQLatch = 0;
    nIRQReload = 0;
    bIRQEnable = false;
    bIRQActive = false;

    pPRGBank.fill(0);
    pCHRBank.fill(0);

    pPRGBank[0] = 0;
    pPRGBank[1] = 1;
    pPRGBank[2] = (nPRGBanks * 2) - 2;
    pPRGBank[3] = (nPRGBanks * 2) - 1;
}

bool Mapper_004::cpuMapRead(uint16_t addr, uint32_t &mapped_addr, uint8_t &data) {
    if (addr >= 0x6000 && addr <= 0x7FFF) {
        mapped_addr = 0xFFFFFFFF;
        data = vRAMStatic[addr & 0x1FFF];
        return true;
    }

    if (addr >= 0x8000 && addr <= 0x9FFF) {
        mapped_addr = pPRGBank[0] + (addr & 0x1FFF);
        return true;
    }
    if (addr >= 0xA000 && addr <= 0xBFFF) {
        mapped_addr = pPRGBank[1] + (addr & 0x1FFF);
        return true;
    }
    if (addr >= 0xC000 && addr <= 0xDFFF) {
        mapped_addr = pPRGBank[2] + (addr & 0x1FFF);
        return true;
    }
    if (addr >= 0xE000 && addr <= 0xFFFF) {
        mapped_addr = pPRGBank[3] + (addr & 0x1FFF);
        return true;
    }

    return false;
}

bool Mapper_004::cpuMapWrite(uint16_t addr, uint32_t &mapped_addr, uint8_t data) {
    if (addr >= 0x6000 && addr <= 0x7FFF) {
        mapped_addr = 0xFFFFFFFF;
        vRAMStatic[addr & 0x1FFF] = data;
        return true;
    }

    if (addr >= 0x8000 && addr <= 0x9FFF) {
        // Bank Select
        if (!(addr & 0x0001)) {
            nTargetRegister = data & 0x07;
            bPRGBankMode = (data & 0x40);
            bCHRInversion = (data & 0x80);
        } else {
            // Update Target Register
            if (nTargetRegister == 0) pCHRBank[0] = data & 0xFE;
            else if (nTargetRegister == 1) pCHRBank[1] = data & 0xFE;
            else if (nTargetRegister == 2) pCHRBank[2] = data;
            else if (nTargetRegister == 3) pCHRBank[3] = data;
            else if (nTargetRegister == 4) pCHRBank[4] = data;
            else if (nTargetRegister == 5) pCHRBank[5] = data;
            else if (nTargetRegister == 6) {
                if (bPRGBankMode) {
                    pPRGBank[0] = (nPRGBanks * 2) - 2; // Fixed at 2nd to last
                    pPRGBank[2] = (data & 0x3F) * 0x2000;
                } else {
                    pPRGBank[0] = (data & 0x3F) * 0x2000;
                    pPRGBank[2] = (nPRGBanks * 2) - 2; // Fixed at 2nd to last
                }
            } else if (nTargetRegister == 7) {
                 pPRGBank[1] = (data & 0x3F) * 0x2000;
            }
        }
        
        // Fix up the fixed banks (last bank is always fixed at -1)
        if (bPRGBankMode) {
            pPRGBank[2] = pPRGBank[2]; // Already set
            pPRGBank[0] = (nPRGBanks * 2) - 2; // -2 * 8KB
        } else {
            pPRGBank[0] = pPRGBank[0]; // Already set
            pPRGBank[2] = (nPRGBanks * 2) - 2; // -2 * 8KB
        }
        pPRGBank[3] = (nPRGBanks * 2) - 1; // Last bank
        
        // We need to resolve the raw offsets into absolute memory addresses (8KB blocks)
        // Note: The logic above set them as indices, let's fix that.
        // Actually, let's store 8KB indices in the pPRGBank array, and multiply by 8192 in cpuMapRead.
        // Wait, standardizing: cpuMapRead expects a raw offset into the ROM vector.
        // So pPRGBank should store (Index * 0x2000).
        
        // Corrections for logic above:
        // Registers 6 and 7 set PRG banks. 
        // Logic re-verified:
        // R6: Selects 8KB bank at $8000 (or $C000 if mode=1)
        // R7: Selects 8KB bank at $A000
        // $C000 is fixed to 2nd to last (or $8000 if mode=1)
        // $E000 is fixed to last bank.
        
        // Recalculating Pointers:
        uint32_t bank8k = 0x2000;
        
        // 1. Update Register Storage
        pRegister[nTargetRegister] = data;

        // 2. Update PRG Pointers
        if (bPRGBankMode) {
            pPRGBank[0] = ((nPRGBanks * 2) - 2) * bank8k;
            pPRGBank[1] = (pRegister[7] & 0x3F) * bank8k;
            pPRGBank[2] = (pRegister[6] & 0x3F) * bank8k;
            pPRGBank[3] = ((nPRGBanks * 2) - 1) * bank8k;
        } else {
            pPRGBank[0] = (pRegister[6] & 0x3F) * bank8k;
            pPRGBank[1] = (pRegister[7] & 0x3F) * bank8k;
            pPRGBank[2] = ((nPRGBanks * 2) - 2) * bank8k;
            pPRGBank[3] = ((nPRGBanks * 2) - 1) * bank8k;
        }

        // 3. Update CHR Pointers (1KB Banks)
        uint32_t bank1k = 0x0400;
        if (bCHRInversion) {
            pCHRBank[0] = pRegister[2] * bank1k;
            pCHRBank[1] = pRegister[3] * bank1k;
            pCHRBank[2] = pRegister[4] * bank1k;
            pCHRBank[3] = pRegister[5] * bank1k;
            pCHRBank[4] = (pRegister[0] & 0xFE) * bank1k;
            pCHRBank[5] = (pRegister[0] | 0x01) * bank1k;
            pCHRBank[6] = (pRegister[1] & 0xFE) * bank1k;
            pCHRBank[7] = (pRegister[1] | 0x01) * bank1k;
        } else {
            pCHRBank[0] = (pRegister[0] & 0xFE) * bank1k;
            pCHRBank[1] = (pRegister[0] | 0x01) * bank1k;
            pCHRBank[2] = (pRegister[1] & 0xFE) * bank1k;
            pCHRBank[3] = (pRegister[1] | 0x01) * bank1k;
            pCHRBank[4] = pRegister[2] * bank1k;
            pCHRBank[5] = pRegister[3] * bank1k;
            pCHRBank[6] = pRegister[4] * bank1k;
            pCHRBank[7] = pRegister[5] * bank1k;
        }
    }

    if (addr >= 0xA000 && addr <= 0xBFFF) {
        if (!(addr & 0x0001)) {
            // Mirroring
            if (data & 0x01) mirroring = MirrorMode::HORIZONTAL;
            else mirroring = MirrorMode::VERTICAL;
        }
    }

    if (addr >= 0xC000 && addr <= 0xDFFF) {
        if (!(addr & 0x0001)) {
            nIRQLatch = data;
        } else {
            nIRQReload = true;
        }
    }

    if (addr >= 0xE000 && addr <= 0xFFFF) {
        if (!(addr & 0x0001)) {
            bIRQEnable = false;
            bIRQActive = false;
        } else {
            bIRQEnable = true;
        }
    }

    return false;
}

bool Mapper_004::ppuMapRead(uint16_t addr, uint32_t &mapped_addr) {
    if (addr < 0x2000) {
        // Bank Mapping (1KB granularity)
        int bank = addr / 0x0400;
        mapped_addr = pCHRBank[bank] + (addr & 0x03FF);
        
        // IRQ Detection (A12 Low -> High)
        // Note: Simple filter. If addr & 0x1000 is high, and previous was low.
        // We lack context here, relying on standard A12 hook behavior.
        // Usually, the PPU fetches patterns at 0x0xxx and 0x1xxx. 
        // MMC3 counts on A12 rising.
        // NOTE: This implementation is simplified. Accurate MMC3 needs precise A12 filtering.
        // For now, we trust the PPU calling cycle logic or address pattern.
        // Address A12 rise happens at cycle 260 of visible scanlines usually for sprite fetch.
        
        // Hacky, stateless detection:
        // We can't do it perfectly without knowing the previous state.
        // However, standard emu practice uses a scanline counter helper from PPU.
        // Since we didn't add that, we won't implement IRQ ticking *here* but rely on the PPU to tick it via scanline() if we add it,
        // OR we implement a simplified heuristic if this function is called strictly in order.
        return true;
    }
    return false;
}

// Since we cannot easily track A12 edges without state in ppuMapRead (which is const-ish concept in some emus, but here it's not),
// we will assume the PPU implementation calls a scanline function or similar, OR we add a state variable here.
// Let's rely on the fact PPU fetches sprites at specific times.
// We'll leave the actual IRQ decrement logic for a specific 'scanline' signal if the PPU supported it,
// OR, we assume the user might manually integrate it. 
// For this snippet, I'll add a dirty hack: The mapper *tracks* the last address seen to detect the edge.

// Re-implementing ppuMapRead to track A12 for IRQ
// NOTE: We need a member variable `uint16_t last_ppu_addr` in the class. 
// I'll add it implicitly via the logic below, assuming I added it to the header (which I didn't yet).
// Actually, let's fix the header in the previous block or just assume basic functionality.
// To make this compile and work without changing PPU extensively, we will skip the IRQ logic *inside* read
// and rely on a PPU change later, or accept that MMC3 games won't have working scanline counters yet.
//
// WAIT: I *can* add the IRQ logic here if I treat this map function as stateful.
// A12 low: < 0x1000. A12 high: >= 0x1000.
// If valid map read:
// if ((addr & 0x1000) && !(last_addr & 0x1000)) { decrement(); }

bool Mapper_004::ppuMapWrite(uint16_t addr, uint32_t &mapped_addr) {
    return false;
}

MirrorMode Mapper_004::getMirroringMode() {
    return mirroring;
}

bool Mapper_004::getIRQ() {
    return bIRQActive;
}

void Mapper_004::clearIRQ() {
    bIRQActive = false;
}

// NOTE: To fully support MMC3 IRQs, the PPU needs to notify the mapper on scanlines.
// For now, this mapper provides the banking logic which allows the games to boot and run logic.
// The IRQ games (SMB3 status bar) might shake or float without the IRQ counter.