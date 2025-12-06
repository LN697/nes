#include "ppu.hpp"
#include "logger.hpp"
#include <iostream>
#include <cstring>

// Standard 2C02 System Palette
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

PPU::PPU() {
    tblName.fill(0);
    tblPalette.fill(0);
    pixels.resize(256 * 240);
    pixels.assign(256 * 240, 0xFF000000);
    log("PPU", "PPU initialized (Corrected Shift Timing).");
}

PPU::~PPU() = default;

void PPU::setMirroring(Mirroring mode) { mirroring = mode; }
const std::vector<uint32_t>& PPU::getScreen() const { return pixels; }
const std::array<uint8_t, 256>& PPU::getOAM() const { return oamData; }
void PPU::setCHR(const std::vector<uint8_t>& rom) { chrROM = rom; if (chrROM.size() < 8192) chrROM.resize(8192); }

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
        case 0x0004: data = oamData[oamaddr]; break;
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
            t_ram_addr = (t_ram_addr & 0xF3FF) | ((data & 0x03) << 10);
            break;
        case 0x0001: // MASK
            ppumask = data;
            break;
        case 0x0003: oamaddr = data; break;
        case 0x0004: oamData[oamaddr++] = data; break;
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
                t_ram_addr = (t_ram_addr & 0x00FF) | ((data & 0x3F) << 8);
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
// SCROLLING HELPERS
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

    for (int i = 0; i < cycles; ++i) {
        
        // --- VISIBLE SCANLINES (-1 to 239) ---
        if (scanline >= -1 && scanline <= 239) {
            
            // Skip "Dot 0"
            if (scanline == 0 && cycle == 0) cycle = 1;

            // --- 1. RENDER PIXEL (Immediate) ---
            if (scanline >= 0 && scanline <= 239 && cycle >= 1 && cycle <= 256) {
                renderPixel();
            }

            // --- 2. UPDATE SHIFTERS (After Render) ---
            // Only update if in the fetch phase
            if ((cycle >= 1 && cycle <= 256) || (cycle >= 321 && cycle <= 337)) {
                updateShifters();
                
                // --- 3. FETCH DATA ---
                switch ((cycle - 1) % 8) {
                    case 0:
                        loadBackgroundShifters();
                        bg_next_tile_id = ppuRead(0x2000 | (v_ram_addr & 0x0FFF));
                        break;
                    case 2:
                        {
                            uint16_t tile_x = v_ram_addr & 0x001F;
                            uint16_t tile_y = (v_ram_addr & 0x03E0) >> 5;
                            uint16_t nt_idx = (v_ram_addr & 0x0C00) >> 10;
                            uint16_t attr_addr = 0x23C0 | (nt_idx << 10) | ((tile_y / 4) << 3) | (tile_x / 4);
                            uint8_t attr_byte = ppuRead(attr_addr);
                            bool bottom = tile_y & 2;
                            bool right  = tile_x & 2;
                            uint8_t shift = (bottom ? 4 : 0) | (right ? 2 : 0);
                            bg_next_tile_attrib = (attr_byte >> shift) & 0x03;
                        }
                        break;
                    case 4:
                        {
                            uint16_t pattern_base = (ppuctrl & 0x10) ? 0x1000 : 0x0000;
                            uint16_t tile_addr = pattern_base + (static_cast<uint16_t>(bg_next_tile_id) * 16) + ((v_ram_addr >> 12) & 0x07);
                            bg_next_tile_lsb = ppuRead(tile_addr);
                        }
                        break;
                    case 6:
                        {
                            uint16_t pattern_base = (ppuctrl & 0x10) ? 0x1000 : 0x0000;
                            uint16_t tile_addr = pattern_base + (static_cast<uint16_t>(bg_next_tile_id) * 16) + ((v_ram_addr >> 12) & 0x07);
                            bg_next_tile_msb = ppuRead(tile_addr + 8);
                        }
                        break;
                    case 7:
                        incrementScrollX();
                        break;
                }
            }

            if (cycle == 256) {
                incrementScrollY();
            }

            if (cycle == 257) {
                loadBackgroundShifters();
                transferAddressX();
            }

            if (scanline == -1 && cycle >= 280 && cycle <= 304) {
                transferAddressY();
            }
        }

        if (scanline == 241 && cycle == 1) {
            ppustatus |= 0x80; 
            if (ppuctrl & 0x80) nmiOccurred = true;
            frame_done = true;
        }
        
        if (scanline == -1 && cycle == 1) {
            ppustatus &= ~(0x80 | 0x40 | 0x20); 
            nmiOccurred = false; 
        }

        cycle++;
        if (cycle >= 341) {
            cycle = 0;
            scanline++;
            if (scanline >= 262) {
                scanline = -1;
                frame_complete = true;
            }
        }
    }
    return frame_done;
}

// =============================================================
// PIXEL PIPELINE
// =============================================================

void PPU::renderPixel() {
    uint8_t bg_pixel = 0x00;
    uint8_t bg_palette = 0x00;
    bool bg_opaque = false;

    if (ppumask & 0x08) {
        // Mux selects the bit. fine_x = 0 -> Bit 15.
        uint16_t bit_mux = 0x8000 >> fine_x;

        uint8_t p0 = (bg_shifter_pattern_lo & bit_mux) > 0;
        uint8_t p1 = (bg_shifter_pattern_hi & bit_mux) > 0;
        bg_pixel = (p1 << 1) | p0;

        uint8_t pal0 = (bg_shifter_attrib_lo & bit_mux) > 0;
        uint8_t pal1 = (bg_shifter_attrib_hi & bit_mux) > 0;
        bg_palette = (pal1 << 1) | pal0;

        if (bg_pixel != 0) bg_opaque = true;
    }

    uint8_t sp_pixel = 0x00;
    uint8_t sp_palette = 0x00;
    bool sp_priority = false; 
    bool sp_0_hit = false;

    if (ppumask & 0x10) {
        for (int i = 0; i < 64; i++) {
            uint8_t y = oamData[i * 4 + 0];
            int diff_y = scanline - (y + 1);
            int height = (ppuctrl & 0x20) ? 16 : 8;
            
            if (diff_y >= 0 && diff_y < height) {
                uint8_t tile_idx = oamData[i * 4 + 1];
                uint8_t attr     = oamData[i * 4 + 2];
                uint8_t x        = oamData[i * 4 + 3];
                int diff_x = (cycle - 1) - x;
                
                if (diff_x >= 0 && diff_x < 8) {
                    bool flip_h = attr & 0x40;
                    bool flip_v = attr & 0x80;
                    if (flip_h) diff_x = 7 - diff_x;
                    if (flip_v) diff_y = height - 1 - diff_y;
                    
                    uint16_t ptrn_addr = 0;
                    if (height == 8) {
                         uint16_t base = (ppuctrl & 0x08) ? 0x1000 : 0x0000;
                         ptrn_addr = base + (tile_idx * 16) + diff_y;
                    } else {
                         uint16_t base = (tile_idx & 1) ? 0x1000 : 0x0000;
                         ptrn_addr = base + ((tile_idx & 0xFE) * 16) + diff_y;
                         if (diff_y >= 8) ptrn_addr += 16; 
                    }
                    
                    uint8_t p_lo = ppuRead(ptrn_addr);
                    uint8_t p_hi = ppuRead(ptrn_addr + 8);
                    uint8_t bit = 0x80 >> diff_x;
                    uint8_t pix = ((p_hi & bit) ? 2 : 0) | ((p_lo & bit) ? 1 : 0);
                    
                    if (pix != 0) {
                        sp_pixel = pix;
                        sp_palette = (attr & 0x03) + 4;
                        sp_priority = (attr & 0x20) != 0;
                        if (i == 0) sp_0_hit = true;
                        break; 
                    }
                }
            }
        }
    }

    if (bg_opaque && sp_0_hit && (ppumask & 0x18) && (cycle - 1) != 255) {
        if (!(ppustatus & 0x40)) {
            ppustatus |= 0x40;
        }
    }

    uint32_t final_color = 0;
    if (bg_pixel == 0 && sp_pixel == 0) final_color = getColorFromPaletteRam(0, 0);
    else if (bg_pixel == 0 && sp_pixel > 0) final_color = getColorFromPaletteRam(sp_palette, sp_pixel);
    else if (bg_pixel > 0 && sp_pixel == 0) final_color = getColorFromPaletteRam(bg_palette, bg_pixel);
    else {
        if (sp_priority) final_color = getColorFromPaletteRam(bg_palette, bg_pixel);
        else             final_color = getColorFromPaletteRam(sp_palette, sp_pixel);
    }

    int idx = scanline * 256 + (cycle - 1);
    if (idx < 256 * 240) pixels[idx] = final_color;
}

uint32_t PPU::getColorFromPaletteRam(uint8_t palette, uint8_t pixel) {
    uint16_t addr = 0x3F00 + (palette * 4) + pixel;
    if (pixel == 0) addr = 0x3F00; 
    uint8_t idx = ppuRead(addr) & 0x3F;
    return systemPalette[idx];
}

uint8_t PPU::ppuRead(uint16_t address) {
    address &= 0x3FFF;
    if (address < 0x2000) return chrROM[address]; 
    if (address < 0x3F00) { 
        address &= 0x0FFF;
        if (mirroring == Mirroring::VERTICAL) {
            return tblName[address & 0x07FF];
        } 
        else if (mirroring == Mirroring::HORIZONTAL) {
            if (address & 0x0800) return tblName[0x0400 + (address & 0x03FF)];
            else                  return tblName[address & 0x03FF];
        }
    }
    if (address >= 0x3F00) { 
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
    if (address < 0x2000) chrROM[address] = data; 
    else if (address < 0x3F00) {
        address &= 0x0FFF;
        if (mirroring == Mirroring::VERTICAL) {
            tblName[address & 0x07FF] = data;
        } 
        else if (mirroring == Mirroring::HORIZONTAL) {
            if (address & 0x0800) tblName[0x0400 + (address & 0x03FF)] = data;
            else                  tblName[address & 0x03FF] = data;
        }
    }
    else if (address >= 0x3F00) {
        address &= 0x001F;
        if (address == 0x0010) address = 0x0000;
        if (address == 0x0014) address = 0x0004;
        if (address == 0x0018) address = 0x0008;
        if (address == 0x001C) address = 0x000C;
        tblPalette[address] = data;
    }
}