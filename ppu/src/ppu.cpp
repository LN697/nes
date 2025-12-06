#include "ppu.hpp"
#include "logger.hpp"
#include <iostream>

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

void PPU::startOAMDMA(const std::array<uint8_t, 256>& data) {
    for (int i = 0; i < 256; ++i) {
        oamData[(oamaddr + i) & 0xFF] = data[i];
    }
}

void PPU::step(int cycles) {
    cycle += cycles;
    
    // NTSC Timing
    if (cycle >= 341) {
        cycle -= 341;
        scanline++;

        if (scanline >= 262) {
            scanline = 0;
            nmiOccurred = false;
            ppustatus &= ~0x80; // Clear VBlank
            ppustatus &= ~0x40; // Clear Sprite 0 Hit at start of frame
        }
    }

    // [NEW] Sprite 0 Hit Detection
    // Checks if we are currently drawing the pixel where the hit occurs.
    // Optimization: Only check if rendering is enabled (Mask bits 3 or 4 set)
    // and we haven't already hit it this frame.
    if ((ppumask & 0x18) && !(ppustatus & 0x40)) {
        if (spriteZeroHit(cycle, scanline)) {
            ppustatus |= 0x40; // Set Sprite 0 Hit flag
        }
    }

    // Set VBlank flag
    if (scanline == 241 && cycle == 1) {
        ppustatus |= 0x80;
        if (ppuctrl & 0x80) {
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

bool PPU::spriteZeroHit(int currentCycle, int currentScanline) {
    // 1. Get Sprite 0 coordinates
    uint8_t y = oamData[0];
    uint8_t tileID = oamData[1];
    uint8_t attr = oamData[2];
    uint8_t x = oamData[3];

    // 2. Bounding Box Check
    // Sprite 0 is drawn on scanlines Y+1 to Y+8 (or Y+16)
    // We are currently at 'currentScanline' and 'currentCycle' (which approximates X)
    
    // Check Y range
    if (currentScanline < y + 1 || currentScanline > y + 8) return false;
    
    // Check X range (cycle roughly corresponds to X pixel)
    // Note: Cycles 1-256 are pixels 0-255.
    if (currentCycle < x || currentCycle > x + 8) return false;

    // 3. Pixel Perfect Check
    // If we are inside the box, we must check if both pixels are opaque.
    
    // A. Fetch Sprite Pixel
    uint16_t spritePatternBase = (ppuctrl & 0x08) ? 0x1000 : 0x0000;
    int row = currentScanline - (y + 1);
    // Handle Flip V (Bit 7)
    if (attr & 0x80) row = 7 - row;
    
    uint16_t spriteAddr = spritePatternBase + (tileID * 16) + row;
    uint8_t s_lsb = ppuRead(spriteAddr);
    uint8_t s_msb = ppuRead(spriteAddr + 8);
    
    // Handle Flip H (Bit 6)
    int col = currentCycle - x;
    if (attr & 0x40) col = 7 - col;
    
    uint8_t spritePixel = ((s_lsb >> (7 - col)) & 1) | (((s_msb >> (7 - col)) & 1) << 1);
    
    // If sprite pixel is transparent, no hit.
    if (spritePixel == 0) return false;

    // B. Fetch Background Pixel
    // Reconstruct the background pixel at the current (X, Y) using Scroll Registers.
    
    // If BG rendering is disabled in Mask (Bit 3), the BG is transparent.
    if (!(ppumask & 0x08)) return false;

    // 1. Calculate "World Space" Coordinates
    // We use v_ram_addr (Loopy V) to determine the top-left scroll position.
    // Coarse X: bits 0-4, Coarse Y: bits 5-9, NT: bits 10-11, Fine Y: bits 12-14
    
    uint16_t scrollX = (v_ram_addr & 0x001F) * 8 + fine_x + currentCycle;
    uint16_t scrollY = ((v_ram_addr >> 5) & 0x001F) * 8 + ((v_ram_addr >> 12) & 0x07) + currentScanline;

    // 2. Handle Nametable Wrapping
    // Determine which nametable we are currently in based on the scroll + screen position
    uint8_t nt = (v_ram_addr >> 10) & 0x03;
    
    if (scrollX >= 256) {
        scrollX %= 256;
        nt ^= 0x01; // Cross horizontal boundary
    }
    if (scrollY >= 240) {
        scrollY %= 240;
        nt ^= 0x02; // Cross vertical boundary
    }

    // 3. Fetch Tile ID
    // Address: $2000 | (NT << 10) | (TileY << 5) | TileX
    uint16_t tileX = scrollX / 8;
    uint16_t tileY = scrollY / 8;
    uint16_t tileAddr = 0x2000 | (nt << 10) | (tileY << 5) | tileX;
    uint8_t bgTileID = ppuRead(tileAddr);

    // 4. Fetch Pattern Data
    // BG Pattern Table is PPUCTRL Bit 4 (0x0000 or 0x1000)
    uint16_t bgPatternBase = (ppuctrl & 0x10) ? 0x1000 : 0x0000;
    uint8_t fineY = scrollY % 8;
    
    uint16_t patternAddr = bgPatternBase + (bgTileID * 16) + fineY;
    uint8_t bg_lsb = ppuRead(patternAddr);
    uint8_t bg_msb = ppuRead(patternAddr + 8);

    // 5. Extract Pixel
    // Bit selection depends on Fine X alignment
    uint8_t fineX_bit = 7 - (scrollX % 8);
    uint8_t bgPixel = ((bg_lsb >> fineX_bit) & 0x01) | 
                      (((bg_msb >> fineX_bit) & 0x01) << 1);

    // 6. Check Collision
    // A hit requires BOTH the Sprite Pixel AND Background Pixel to be opaque (non-zero).
    if (bgPixel == 0) return false;

    return true; 
}

uint8_t PPU::getFineX() const {
    return fine_x;
}