#include "ppu.hpp"
#include <cstring>
#include <algorithm>

// =============================================================
// STATIC DATA
// =============================================================

const std::array<uint32_t, 64> PPU::systemPalette = {{
    0xFF7C7C7C, 0xFF0000FC, 0xFF0000BC, 0xFF4428BC, 0xFF940084, 0xFFA80020, 0xFFA81000, 0xFF881400,
    0xFF503000, 0xFF007800, 0xFF006800, 0xFF005800, 0xFF004058, 0xFF000000, 0xFF000000, 0xFF000000,
    0xFFBCBCBC, 0xFF0078F8, 0xFF0058F8, 0xFF6844FC, 0xFFD800CC, 0xFFE40058, 0xFFF83800, 0xFFE45C10,
    0xFFAC7C00, 0xFF00B800, 0xFF00A800, 0xFF00A844, 0xFF008888, 0xFF000000, 0xFF000000, 0xFF000000,
    0xFFF8F8F8, 0xFF3CBCFC, 0xFF6888FC, 0xFF9878F8, 0xFFF878F8, 0xFFF85898, 0xFFF87858, 0xFFFCA044,
    0xFFF8B800, 0xFFB8F818, 0xFF58D854, 0xFF58F898, 0xFF00E8D8, 0xFF787878, 0xFF000000, 0xFF000000,
    0xFFFCFCFC, 0xFFA4E4FC, 0xFFB8B8F8, 0xFFD8B8F8, 0xFFF8B8F8, 0xFFF8A4C0, 0xFFF0D0B0, 0xFFFCE0A8,
    0xFFF8D878, 0xFFD8F878, 0xFFB8F8B8, 0xFFB8F8D8, 0xFF00FCFC, 0xFFF8D8F8, 0xFF000000, 0xFF000000
}};

// =============================================================
// CONSTRUCTOR / DESTRUCTOR
// =============================================================

PPU::PPU() {
    tblName.fill(0);
    tblPalette.fill(0);
    oamData.fill(0);
    pixels.resize(256 * 240);
    pixels.assign(256 * 240, 0xFF000000);
    spriteScanline.reserve(8);
    
    // Default to Vertical as it is safer for common games
    mirroring = Mirroring::VERTICAL;
}

PPU::~PPU() = default;

// =============================================================
// CONFIGURATION
// =============================================================

void PPU::setMirroring(Mirroring mode) { 
    mirroring = mode; 
}

void PPU::setCHR(const std::vector<uint8_t>& rom) { 
    chrROM = rom; 
    if (chrROM.size() < 8192) {
        chrROM.resize(8192); 
    }
}

const std::vector<uint32_t>& PPU::getScreen() const { 
    return pixels;
}

const std::array<uint8_t, 256>& PPU::getOAM() const { 
    return oamData;
}

// =============================================================
// MEMORY INTERFACE (INTERNAL)
// =============================================================

uint8_t PPU::ppuRead(uint16_t address) {
    address &= 0x3FFF;
    
    if (address < 0x2000) {
        return chrROM[address]; 
    }
    
    if (address < 0x3F00) { 
        address &= 0x0FFF;
        if (mirroring == Mirroring::VERTICAL) {
            return tblName[address & 0x07FF];
        } 
        else if (mirroring == Mirroring::HORIZONTAL) {
            if (address & 0x0800) return tblName[0x0400 + (address & 0x03FF)]; 
            else return tblName[address & 0x03FF];
        }
    }
    
    if (address >= 0x3F00) { 
        address &= 0x001F;
        if ((address & 0x03) == 0) address &= 0x0F;
        return tblPalette[address];
    }
    
    return 0;
}

void PPU::ppuWrite(uint16_t address, uint8_t data) {
    address &= 0x3FFF;
    
    if (address < 0x2000) {
        chrROM[address] = data; 
    }
    else if (address < 0x3F00) {
        address &= 0x0FFF;
        if (mirroring == Mirroring::VERTICAL) {
            tblName[address & 0x07FF] = data;
        } 
        else if (mirroring == Mirroring::HORIZONTAL) {
            if (address & 0x0800) tblName[0x0400 + (address & 0x03FF)] = data; 
            else tblName[address & 0x03FF] = data;
        }
    }
    else if (address >= 0x3F00) {
        address &= 0x001F;
        if ((address & 0x03) == 0) address &= 0x0F;
        tblPalette[address] = data;
    }
}

// =============================================================
// CPU INTERFACE
// =============================================================

uint8_t PPU::cpuRead(uint16_t address) {
    uint8_t data = 0x00;
    switch (address & 0x0007) {
        case 0x0002: // STATUS
            data = (ppustatus & 0xE0) | (ppu_data_buffer & 0x1F);
            ppustatus &= ~0x80;
            address_latch = false;
            break;
        case 0x0004: // OAM DATA
            data = oamData[oamaddr];
            break;
        case 0x0007: // DATA
            data = ppu_data_buffer;
            ppu_data_buffer = ppuRead(v_ram_addr);
            if (v_ram_addr >= 0x3F00) data = ppu_data_buffer;
            v_ram_addr += (ppuctrl & 0x04) ? 32 : 1;
            break;
    }
    return data;
}

void PPU::cpuWrite(uint16_t address, uint8_t data) {
    switch (address & 0x0007) {
        case 0x0000: // CTRL
            ppuctrl = data;
            t_ram_addr = (t_ram_addr & 0xF3FF) | ((static_cast<uint16_t>(data) & 0x03) << 10);
            break;
        case 0x0001: // MASK
            ppumask = data;
            break;
        case 0x0003: // OAMADDR
            oamaddr = data; 
            break;
        case 0x0004: // OAMDATA
            oamData[oamaddr++] = data; 
            break;
        case 0x0005: // SCROLL
            if (!address_latch) {
                fine_x = data & 0x07;
                t_ram_addr = (t_ram_addr & 0xFFE0) | ((data >> 3) & 0x1F);
                address_latch = true;
            } else {
                t_ram_addr = (t_ram_addr & 0x8FFF) | ((data & 0x07) << 12);
                t_ram_addr = (t_ram_addr & 0xFC1F) | ((data & 0xF8) << 2);
                address_latch = false;
            }
            break;
        case 0x0006: // ADDR
            if (!address_latch) {
                t_ram_addr = (t_ram_addr & 0x00FF) | ((static_cast<uint16_t>(data) & 0x3F) << 8);
                address_latch = true;
            } else {
                t_ram_addr = (t_ram_addr & 0xFF00) | data;
                v_ram_addr = t_ram_addr;
                address_latch = false;
            }
            break;
        case 0x0007: // DATA
            ppuWrite(v_ram_addr, data);
            v_ram_addr += (ppuctrl & 0x04) ? 32 : 1;
            break;
    }
}

void PPU::startOAMDMA(const std::array<uint8_t, 256>& data) {
    for (int i = 0; i < 256; ++i) {
        oamData[(oamaddr + i) & 0xFF] = data[i];
    }
}

// =============================================================
// PIPELINE HELPERS
// =============================================================

void PPU::incrementScrollX() {
    if ((v_ram_addr & 0x001F) == 31) {
        v_ram_addr &= ~0x001F;
        v_ram_addr ^= 0x0400;
    } else {
        v_ram_addr += 1;
    }
}

void PPU::incrementScrollY() {
    if ((v_ram_addr & 0x7000) != 0x7000) {
        v_ram_addr += 0x1000;
    } else {
        v_ram_addr &= ~0x7000;
        int y = (v_ram_addr & 0x03E0) >> 5;
        if (y == 29) {
            y = 0;
            v_ram_addr ^= 0x0800;
        } else if (y == 31) {
            y = 0;
        } else {
            y += 1;
        }
        v_ram_addr = (v_ram_addr & ~0x03E0) | (y << 5);
    }
}

void PPU::transferAddressX() {
    v_ram_addr = (v_ram_addr & 0xFBE0) | (t_ram_addr & 0x041F);
}

void PPU::transferAddressY() {
    v_ram_addr = (v_ram_addr & 0x841F) | (t_ram_addr & 0x7BE0);
}

void PPU::loadBackgroundShifters() {
    bg_shifter_pattern_lo = (bg_shifter_pattern_lo & 0xFF00) | bg_next_tile_lsb;
    bg_shifter_pattern_hi = (bg_shifter_pattern_hi & 0xFF00) | bg_next_tile_msb;
    bg_shifter_attrib_lo  = (bg_shifter_attrib_lo & 0xFF00) | ((bg_next_tile_attrib & 0b01) ? 0xFF : 0x00);
    bg_shifter_attrib_hi  = (bg_shifter_attrib_hi & 0xFF00) | ((bg_next_tile_attrib & 0b10) ? 0xFF : 0x00);
}

void PPU::updateShifters() {
    if (ppumask & 0x18) { 
        bg_shifter_pattern_lo <<= 1;
        bg_shifter_pattern_hi <<= 1;
        bg_shifter_attrib_lo <<= 1;
        bg_shifter_attrib_hi <<= 1;
    }
}

// =============================================================
// MAIN CYCLE LOOP
// =============================================================

bool PPU::step(int cycles) {
    bool frame_done = false;

    // Optimization: Loop unrolling is not necessary here as PPU logic is complex enough
    // to stall pipeline anyway. Keeping it simple and robust.
    for (int i = 0; i < cycles; ++i) {
        
        // --- VISIBLE FRAME ---
        if (scanline >= -1 && scanline <= 239) {
            
            // Clean Status on Pre-render
            if (scanline == -1 && cycle == 1) {
                ppustatus &= ~(0xE0); // Clear VBlank, Sprite0, Overflow
                nmiOccurred = false; 
                bSpriteZeroHitPossible = false;
            }

            // Odd Frame Skip
            if (scanline == 0 && cycle == 0 && (frame_count % 2) && (ppumask & 0x18)) {
                 cycle = 1; 
            }

            // --- RENDER ---
            // Only render visible pixels
            if (scanline >= 0 && scanline <= 239 && cycle >= 1 && cycle <= 256) {
                renderPixel();
            }

            // --- PIPELINE ---
            // Background shift/fetch happens on lines 0-239 AND Pre-render (-1)
            // CRITICAL FIX: Cycle limit changed from 337 to 336. 
            // Cycle 337 is idle. Running it causes an extra shift, corrupting the first tile.
            if ((cycle >= 1 && cycle <= 256) || (cycle >= 321 && cycle <= 336)) {
                updateShifters();
                
                switch ((cycle - 1) % 8) {
                    case 0:
                        loadBackgroundShifters();
                        bg_next_tile_id = ppuRead(0x2000 | (v_ram_addr & 0x0FFF));
                        break;
                    case 2:
                        {
                            // Optimized Attribute Fetch
                            uint16_t tile_x = v_ram_addr & 0x001F;
                            uint16_t tile_y = (v_ram_addr & 0x03E0) >> 5;
                            uint16_t nt_idx = (v_ram_addr & 0x0C00) >> 10;
                            uint16_t attr_addr = 0x23C0 | (nt_idx << 10) | ((tile_y / 4) << 3) | (tile_x / 4);
                            
                            uint8_t shift = ((tile_y & 2) << 1) | (tile_x & 2);
                            bg_next_tile_attrib = (ppuRead(attr_addr) >> shift) & 0x03;
                        }
                        break;
                    case 4:
                        {
                            uint16_t pattern_base = (ppuctrl & 0x10) ? 0x1000 : 0x0000;
                            bg_next_tile_lsb = ppuRead(pattern_base + (static_cast<uint16_t>(bg_next_tile_id) * 16) + ((v_ram_addr >> 12) & 0x07));
                        }
                        break;
                    case 6:
                        {
                            uint16_t pattern_base = (ppuctrl & 0x10) ? 0x1000 : 0x0000;
                            bg_next_tile_msb = ppuRead(pattern_base + (static_cast<uint16_t>(bg_next_tile_id) * 16) + ((v_ram_addr >> 12) & 0x07) + 8);
                        }
                        break;
                    case 7:
                        if (ppumask & 0x18) incrementScrollX();
                        break;
                }
            }

            // --- END OF LINE HOUSEKEEPING ---
            if (cycle == 256) {
                if (ppumask & 0x18) incrementScrollY();
            }

            if (cycle == 257) {
                loadBackgroundShifters();
                if (ppumask & 0x18) transferAddressX();
                
                // Sprite Eval for next line
                if (scanline >= -1 && scanline < 239) evaluateSprites();
            }

            if (scanline == -1 && cycle >= 280 && cycle <= 304) {
                if (ppumask & 0x18) transferAddressY();
            }
        }

        // --- VBLANK ---
        if (scanline == 241 && cycle == 1) {
            ppustatus |= 0x80; 
            if (ppuctrl & 0x80) nmiOccurred = true;
            frame_done = true;
            frame_count++;
        }

        // --- CYCLE ADVANCE ---
        cycle++;
        if (cycle >= 341) {
            cycle = 0;
            scanline++;
            
            // CRITICAL FIX: NTSC Frame is 262 lines (0-261). 
            // Wrap happens at end of 261, jumping to -1.
            // Old code waited for 262, creating a dead line.
            if (scanline >= 261) {
                scanline = -1;
                frame_complete = true;
            }
        }
    }
    return frame_done;
}

// =============================================================
// SPRITE EVALUATION
// =============================================================

void PPU::evaluateSprites() {
    spriteScanline.clear();
    sprite_count = 0;
    
    int next_scanline = (scanline + 1); // Simplification: Wrap handled by next frame logic
    uint8_t sprite_height = (ppuctrl & 0x20) ? 16 : 8;
    bool next_sprite_zero_hit = false;

    // Evaluate all 64 sprites
    for (int i = 0; i < 64; i++) {
        // Optimization: Read OAM direct access
        int y = oamData[i * 4];
        int diff = next_scanline - y;

        if (diff >= 0 && diff < sprite_height) {
            if (sprite_count < 8) {
                ObjectAttrEntry entry;
                entry.y = y;
                entry.id = oamData[i * 4 + 1];
                entry.attribute = oamData[i * 4 + 2];
                entry.x = oamData[i * 4 + 3];
                entry.isZero = (i == 0);
                
                if (i == 0) next_sprite_zero_hit = true;
                
                spriteScanline.push_back(entry);
                sprite_count++;
            } else {
                ppustatus |= 0x20; // Overflow
                break; 
            }
        }
    }
    bSpriteZeroBeingRendered = next_sprite_zero_hit;
}

// =============================================================
// RENDERING
// =============================================================

void PPU::renderPixel() {
    uint8_t bg_pixel = 0x00;
    uint8_t bg_palette = 0x00;
    bool bg_opaque = false;

    int x = cycle - 1; // Current screen X coordinate (0-255)

    // --- 1. BACKGROUND ---
    if (ppumask & 0x08) {
        // Left-side Clipping (Hide first 8 pixels if bit 1 is clear)
        if ((ppumask & 0x02) || (x >= 8)) {
            uint16_t bit_mux = 0x8000 >> fine_x;
            uint8_t p0 = (bg_shifter_pattern_lo & bit_mux) > 0;
            uint8_t p1 = (bg_shifter_pattern_hi & bit_mux) > 0;
            bg_pixel = (p1 << 1) | p0;

            uint8_t pal0 = (bg_shifter_attrib_lo & bit_mux) > 0;
            uint8_t pal1 = (bg_shifter_attrib_hi & bit_mux) > 0;
            bg_palette = (pal1 << 1) | pal0;

            if (bg_pixel != 0) bg_opaque = true;
        }
    }

    // --- 2. SPRITES ---
    uint8_t sp_pixel = 0x00;
    uint8_t sp_palette = 0x00;
    bool sp_priority = false; 
    bool sp_0_rendered = false;

    if (ppumask & 0x10) {
        // Left-side Clipping (Hide first 8 pixels if bit 2 is clear)
        if ((ppumask & 0x04) || (x >= 8)) {
            
            for (const auto& sprite : spriteScanline) {
                int diff_x = x - sprite.x;
                
                if (diff_x >= 0 && diff_x < 8) {
                    uint8_t height = (ppuctrl & 0x20) ? 16 : 8;
                    bool flip_h = sprite.attribute & 0x40;
                    
                    // X-Flip calculation
                    if (flip_h) diff_x = 7 - diff_x;
                    
                    // Fetch Pixel Pattern Bit
                    // Note: We re-fetch here for simplicity. 
                    int diff_y = scanline - sprite.y;
                    if (sprite.attribute & 0x80) diff_y = height - 1 - diff_y; // Y-Flip

                    uint16_t ptrn_addr = 0;
                    if (height == 8) {
                        ptrn_addr = ((ppuctrl & 0x08) ? 0x1000 : 0x0000) + (sprite.id * 16) + diff_y;
                    } else {
                        ptrn_addr = ((sprite.id & 1) ? 0x1000 : 0x0000) + ((sprite.id & 0xFE) * 16) + diff_y;
                        if (diff_y >= 8) ptrn_addr += 16;
                    }

                    uint8_t p_lo = ppuRead(ptrn_addr);
                    uint8_t p_hi = ppuRead(ptrn_addr + 8);
                    uint8_t pix = (((p_hi >> (7 - diff_x)) & 1) << 1) | ((p_lo >> (7 - diff_x)) & 1);
                    
                    if (pix != 0) {
                        sp_pixel = pix;
                        sp_palette = (sprite.attribute & 0x03) + 4;
                        sp_priority = (sprite.attribute & 0x20) == 0;
                        if (sprite.isZero) sp_0_rendered = true;
                        break; 
                    }
                }
            }
        }
    }

    // --- 3. SPRITE ZERO HIT ---
    if (bg_opaque && sp_0_rendered && (ppumask & 0x18)) {
        // Hit requires mask bits to enable rendering at current X
        if (!((ppumask & 0x06) != 0x06 && x < 8)) {
             if (x != 255) ppustatus |= 0x40;
        }
    }

    // --- 4. COMPOSITING ---
    uint32_t final_color;
    if (bg_pixel == 0 && sp_pixel == 0) {
        final_color = getColorFromPaletteRam(0, 0);
    } else if (bg_pixel == 0) {
        final_color = getColorFromPaletteRam(sp_palette, sp_pixel);
    } else if (sp_pixel == 0) {
        final_color = getColorFromPaletteRam(bg_palette, bg_pixel);
    } else {
        final_color = sp_priority ? getColorFromPaletteRam(sp_palette, sp_pixel) 
                                  : getColorFromPaletteRam(bg_palette, bg_pixel);
    }

    // Direct Array Access
    pixels[scanline * 256 + x] = final_color;
}

uint32_t PPU::getColorFromPaletteRam(uint8_t palette, uint8_t pixel) {
    return systemPalette[ppuRead(0x3F00 + (palette * 4) + pixel) & 0x3F];
}
