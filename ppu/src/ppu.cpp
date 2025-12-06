#include "ppu.hpp"
#include "logger.hpp"
#include <iostream> // For error logging if needed

PPU::PPU() {
    tblName.fill(0);
    tblPalette.fill(0);

    log("PPU", "PPU initialized.");
}

PPU::~PPU() = default;

void PPU::setMirroring(Mirroring mode) {
    mirroring = mode;
}

uint8_t PPU::cpuRead(uint16_t address) {
    uint8_t data = 0x00;
    
    // CPU only sees 8 registers, mirrored every 8 bytes from $2000-$3FFF
    switch (address & 0x0007) {
        case 0x0002: // STATUS ($2002)
            // Reading Status clears the VBlank flag and the Address Latch
            data = (ppustatus & 0xE0) | (ppu_data_buffer & 0x1F); // Noise from buffer
            ppustatus &= ~0x80; // Clear VBlank
            address_latch = false; // Reset latch
            break;

        case 0x0004: // OAM DATA ($2004)
            // Does not increment OAMADDR on read
            data = oamData[oamaddr];
            break;

        case 0x0007: // DATA ($2007)
            // Reading Data increments the VRAM address automatically
            data = ppu_data_buffer;
            ppu_data_buffer = ppuRead(v_ram_addr);
            
            // Palette reads are instant (no buffer delay)
            if (v_ram_addr >= 0x3F00) data = ppu_data_buffer;

            // Increment Address (Horizontal vs Vertical mode)
            v_ram_addr += (ppuctrl & 0x04) ? 32 : 1;
            break;
    }
    return data;
}

void PPU::cpuWrite(uint16_t address, uint8_t data) {
    switch (address & 0x0007) {
        case 0x0000: // CONTROL ($2000)
            ppuctrl = data;
            // Extract Nametable address (bits 0-1) into temporary address t
            t_ram_addr = (t_ram_addr & 0xF3FF) | ((data & 0x03) << 10);
            break;

        case 0x0001: // MASK ($2001)
            ppumask = data;
            break;

        case 0x0003: // OAM ADDR ($2003)
            oamaddr = data;
            break;

        case 0x0004: // OAM DATA ($2004)
            writeOAMData(data);
            break;

        case 0x0005: // SCROLL ($2005)
            if (!address_latch) {
                // First Write: Fine X and Coarse X
                fine_x = data & 0x07;
                t_ram_addr = (t_ram_addr & 0xFFE0) | ((data >> 3) & 0x1F);
                address_latch = true;
            } else {
                // Second Write: Fine Y and Coarse Y
                t_ram_addr = (t_ram_addr & 0x8FFF) | ((data & 0x07) << 12); // Fine Y
                t_ram_addr = (t_ram_addr & 0xFC1F) | ((data & 0xF8) << 2);  // Coarse Y
                address_latch = false;
            }
            break;

        case 0x0006: // ADDRESS ($2006)
            if (!address_latch) {
                // First Write: High Byte
                // Clear high byte of t, merge in new high byte (masked to 14 bits)
                t_ram_addr = (t_ram_addr & 0x00FF) | ((data & 0x3F) << 8);
                address_latch = true;
            } else {
                // Second Write: Low Byte
                t_ram_addr = (t_ram_addr & 0xFF00) | data;
                v_ram_addr = t_ram_addr; // Update current address v from t
                address_latch = false;
            }
            break;

        case 0x0007: // DATA ($2007)
            ppuWrite(v_ram_addr, data);
            // Increment Address
            v_ram_addr += (ppuctrl & 0x04) ? 32 : 1;
            break;
    }
}

void PPU::writeOAMData(uint8_t data) {
    oamData[oamaddr] = data;
    oamaddr++; // Increment OAMADDR after write
}

void PPU::step(int cycles) {
    cycle += cycles;
    
    // NTSC Timing: 341 cycles per scanline
    if (cycle >= 341) {
        cycle -= 341;
        scanline++;

        if (scanline >= 262) {
            scanline = 0;
            nmiOccurred = false;
            ppustatus &= ~0x80; // Clear VBlank flag
            // Clear Sprite 0 Hit here too
        }
    }

    // Set VBlank flag at (241, 1)
    if (scanline == 241 && cycle == 1) {
        ppustatus |= 0x80;
        if (ppuctrl & 0x80) { // If NMI enabled
            nmiOccurred = true;
        }
    }
}

uint8_t PPU::ppuRead(uint16_t address) {
    address &= 0x3FFF;
    
    if (address <= 0x1FFF) {
        // Pattern Tables
        if (address < chrROM.size()) {
            return chrROM[address];
        }
        return 0;
    }
    else if (address >= 0x2000 && address <= 0x3EFF) {
        address &= 0x0FFF;
        
        if (mirroring == Mirroring::VERTICAL) {
            return tblName[address & 0x07ff];
        } else if (mirroring == Mirroring::HORIZONTAL) {
            if (address & 0x0800) {
                // Upper Half ($2800+) -> Offset 0x400 in storage (Table 1)
                return tblName[0x0400 + (address & 0x03FF)];
            } else {
                // Lower Half ($2000+) -> Offset 0x000 in storage (Table 0)
                return tblName[address & 0x03FF];
            }
        }
        
        return 0;
    }
    else if (address >= 0x3F00 && address <= 0x3FFF) {
        // Palettes
        address &= 0x001F;
        if (address == 0x0010) address = 0x0000;
        if (address == 0x0014) address = 0x0004;
        if (address == 0x0018) address = 0x0008;
        if (address == 0x001C) address = 0x000C;
        return tblPalette[address];
    }
    return 0;
}

void PPU::ppuWrite(uint16_t address, uint8_t data) {
    address &= 0x3FFF;
    
    if (address >= 0x2000 && address <= 0x3EFF) {
        // Nametables
        address &= 0x0FFF;
        
        if (mirroring == Mirroring::VERTICAL) {
            tblName[address & 0x07FF] = data;
        }
        else if (mirroring == Mirroring::HORIZONTAL) {
            if (address & 0x0800) {
                // Upper Half ($2800+) -> Offset 0x400 in storage (Table 1)
                tblName[0x0400 + (address & 0x03FF)] = data;
            } else {
                // Lower Half ($2000+) -> Offset 0x000 in storage (Table 0)
                tblName[address & 0x03FF] = data;
            }
        }
    }
    else if (address >= 0x3F00 && address <= 0x3FFF) {
        // Palettes
        address &= 0x001F;
        if (address == 0x0010) address = 0x0000;
        if (address == 0x0014) address = 0x0004;
        if (address == 0x0018) address = 0x0008;
        if (address == 0x001C) address = 0x000C;
        tblPalette[address] = data;
    }
}

void PPU::setCHR(const std::vector<uint8_t>& rom) {
    chrROM = rom;

    if (chrROM.size() < 8192) chrROM.resize(8192);
}

uint8_t PPU::getControl() const {
    return ppuctrl;
}

uint8_t PPU::getMask() const {
    return ppumask;
}

const std::array<uint8_t, 256>& PPU::getOAM() const {
    return oamData;
}

uint16_t PPU::getVRAMAddr() const {
    return v_ram_addr;
}

uint8_t PPU::getFineX() const {
    return fine_x;
}