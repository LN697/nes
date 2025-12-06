#include "renderer.hpp"
#include "logger.hpp"

#include <iostream>

// Standard 2C02 System Palette (RGB)
const std::array<Renderer::RGB, 64> Renderer::systemPalette = {{
    {0x7C,0x7C,0x7C}, {0x00,0x00,0xFC}, {0x00,0x00,0xBC}, {0x44,0x28,0xBC},
    {0x94,0x00,0x84}, {0xA8,0x00,0x20}, {0xA8,0x10,0x00}, {0x88,0x14,0x00},
    {0x50,0x30,0x00}, {0x00,0x78,0x00}, {0x00,0x68,0x00}, {0x00,0x58,0x00},
    {0x00,0x40,0x58}, {0x00,0x00,0x00}, {0x00,0x00,0x00}, {0x00,0x00,0x00},
    {0xBC,0xBC,0xBC}, {0x00,0x78,0xF8}, {0x00,0x58,0xF8}, {0x68,0x44,0xFC},
    {0xD8,0x00,0xCC}, {0xE4,0x00,0x58}, {0xF8,0x38,0x00}, {0xE4,0x5C,0x10},
    {0xAC,0x7C,0x00}, {0x00,0xB8,0x00}, {0x00,0xA8,0x00}, {0x00,0xA8,0x44},
    {0x00,0x88,0x88}, {0x00,0x00,0x00}, {0x00,0x00,0x00}, {0x00,0x00,0x00},
    {0xF8,0xF8,0xF8}, {0x3C,0xBC,0xFC}, {0x68,0x88,0xFC}, {0x98,0x78,0xF8},
    {0xF8,0x78,0xF8}, {0xF8,0x58,0x98}, {0xF8,0x78,0x58}, {0xFC,0xA0,0x44},
    {0xF8,0xB8,0x00}, {0xB8,0xF8,0x18}, {0x58,0xD8,0x54}, {0x58,0xF8,0x98},
    {0x00,0xE8,0xD8}, {0x78,0x78,0x78}, {0x00,0x00,0x00}, {0x00,0x00,0x00},
    {0xFC,0xFC,0xFC}, {0xA4,0xE4,0xFC}, {0xB8,0xB8,0xF8}, {0xD8,0xB8,0xF8},
    {0xF8,0xB8,0xF8}, {0xF8,0xA4,0xC0}, {0xF0,0xD0,0xB0}, {0xFC,0xE0,0xA8},
    {0xF8,0xD8,0x78}, {0xD8,0xF8,0x78}, {0xB8,0xF8,0xB8}, {0xB8,0xF8,0xD8},
    {0x00,0xFC,0xFC}, {0xF8,0xD8,0xF8}, {0x00,0x00,0x00}, {0x00,0x00,0x00}
}};

Renderer::Renderer() {
    pixelBuffer.resize(256 * 240);
    priorityMap.resize(256 * 240);
    log("Renderer", "Renderer initialized.");
}

Renderer::~Renderer() {
    if (texture) SDL_DestroyTexture(texture);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
}

bool Renderer::init(const char* title, int width, int height, int scale) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL Error: " << SDL_GetError() << "\n";
        return false;
    }

    window = SDL_CreateWindow(title, 
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
        width * scale, height * scale, 
        SDL_WINDOW_SHOWN);

    if (!window) return false;

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) return false;

    SDL_RenderSetLogicalSize(renderer, width, height);

    texture = SDL_CreateTexture(renderer, 
        SDL_PIXELFORMAT_ARGB8888, 
        SDL_TEXTUREACCESS_STREAMING, 
        width, height);

    return true;
}

bool Renderer::handleEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) return false;
        if (event.type == SDL_KEYDOWN) {
            if (event.key.keysym.sym == SDLK_ESCAPE) return false;
        }
    }
    return true;
}

void Renderer::draw(Bus& bus) {
    for (int i = 0; i < 32; ++i) {
        paletteCache[i] = bus.ppu.ppuRead(0x3F00 + i);
    }

    renderNameTable(bus);
    renderSprites(bus);
    
    SDL_UpdateTexture(texture, nullptr, pixelBuffer.data(), 256 * sizeof(uint32_t));
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, nullptr, nullptr);
    SDL_RenderPresent(renderer);
}

void Renderer::renderPatternTable(int tableIndex, uint8_t /* paletteID */, Bus& bus) {
    // Each pattern table is 4KB (256 tiles)
    // 16x16 grid of tiles
    
    for (uint16_t nTileY = 0; nTileY < 16; nTileY++) {
        for (uint16_t nTileX = 0; nTileX < 16; nTileX++) {
            
            // Linear offset in the pattern table (16 bytes per tile)
            uint16_t nOffset = nTileY * 256 + nTileX * 16;
            
            // Loop through 8 rows of pixels in the tile
            for (uint16_t row = 0; row < 8; row++) {
                
                // Read the two bitplanes
                // Note: We need a direct PPU Read helper on the Bus to bypass palettes!
                // For now, assume bus.read() maps to PPU space if we use the right offset logic
                // Or better: Access PPU directly if friend class, but Bus::read is CPU map.
                // WEAKNESS: Bus::read(0x0000) reads CPU RAM. We need PPU Read.
                // Assuming you add a bus.ppuRead(addr) helper or similar. 
                // Let's use the raw math assuming we can access PPU bus.
                
                // Address in PPU memory
                uint16_t addr = (tableIndex * 0x1000) + nOffset + row;
                
                // TEMPORARY HACK: Since we don't have a direct ppuBusRead(addr) exposed in Bus yet,
                // we assume the cartridge data is available. 
                // Ideally: uint8_t tile_lsb = bus.ppu.ppuRead(addr);
                // But PPU::ppuRead() is private. You should make it public or friend Renderer.
                
                // Let's assume you updated PPU::ppuRead to be public for the Renderer.
                uint8_t tile_lsb = bus.ppu.ppuRead(addr);
                uint8_t tile_msb = bus.ppu.ppuRead(addr + 8);

                // Iterate through 8 pixels in the row
                for (uint16_t col = 0; col < 8; col++) {
                    // Extract bit from LSB (Bit 0) and MSB (Bit 1)
                    // PPU stores pixels 7..0 (left to right)
                    uint8_t pixel = ((tile_lsb & 0x01) << 0) | ((tile_msb & 0x01) << 1);
                    
                    // Shift registers for next pixel
                    tile_lsb >>= 1;
                    tile_msb >>= 1;

                    // Coordinate in the 256x128 buffer
                    // Table 0 is at X=0, Table 1 is at X=128
                    int x = (tableIndex * 128) + (nTileX * 8) + (7 - col);
                    int y = (nTileY * 8) + row;
                    int index = y * 256 + x;

                    // Map 2-bit pixel value to a Color
                    // 0 = Transparent/BG, 1,2,3 = Colors
                    // Simple Debug Palette: 0=Black, 1=Red, 2=Green, 3=White
                    uint32_t color = 0;
                    switch(pixel) {
                        case 0: color = 0xFF000000; break; 
                        case 1: color = 0xFFFF0000; break; 
                        case 2: color = 0xFF00FF00; break; 
                        case 3: color = 0xFFFFFFFF; break; 
                    }
                    
                    pixelBuffer[index] = color;
                }
            }
        }
    }
}

void Renderer::renderNameTable(Bus& bus) {
    // 1. Get Scroll Registers from PPU
    // 'v' (Loopy V) contains: 
    //   Bits 0-4: Coarse X (0-31)
    //   Bits 5-9: Coarse Y (0-29)
    //   Bits 10-11: Base Nametable Select (0-3)
    //   Bits 12-14: Fine Y (0-7)
    uint16_t v = bus.ppu.getVRAMAddr();
    uint8_t fine_x = bus.ppu.getFineX();
    uint8_t ppuctrl = bus.ppu.getControl();
    
    // Extract components
    uint8_t coarseX = v & 0x001F;
    uint8_t coarseY = (v >> 5) & 0x001F;
    uint8_t nametableX = (v >> 10) & 1;
    uint8_t nametableY = (v >> 11) & 1;
    uint8_t fineY = (v >> 12) & 0x07;

    // 2. Determine Pattern Table for Background
    // PPUCTRL Bit 4 (0=$0000, 1=$1000)
    uint16_t bgPatternBase = (ppuctrl & 0x10) ? 0x1000 : 0x0000;

    // 3. Render Screen Pixels (256x240)
    for (int screenY = 0; screenY < 240; screenY++) {
        
        // --- Y-Axis Scrolling Math ---
        // Calculate the absolute Y pixel in "World Space"
        int absoluteY = screenY + (coarseY * 8) + fineY;
        
        // Convert to Tile Grid Y (0-29)
        int tileY = absoluteY / 8;
        
        // Handle Y Wrapping (Vertical Mirroring Logic)
        // If we go past 30 rows (240 pixels), we wrap to the next vertical nametable
        int currentNtY = nametableY + (tileY / 30);
        tileY %= 30;      // Wrap rows 0-29
        currentNtY %= 2;  // Wrap nametable 0-1 (Vertically)
        
        // Fine Y offset inside the tile (0-7)
        int rowInTile = absoluteY % 8;

        for (int screenX = 0; screenX < 256; screenX++) {
            
            // --- X-Axis Scrolling Math ---
            // Calculate absolute X pixel
            int absoluteX = screenX + (coarseX * 8) + fine_x;
            
            // Convert to Tile Grid X (0-31)
            int tileX = absoluteX / 8;
            
            // Handle X Wrapping (Horizontal Scrolling)
            // If we go past 32 columns (256 pixels), we wrap to the neighbor nametable
            int currentNtX = nametableX + (tileX / 32);
            tileX %= 32;      // Wrap columns 0-31
            currentNtX %= 2;  // Wrap nametable 0-1 (Horizontally)
            
            // Resolve Final Nametable ID (0-3)
            // Layout: 0=(0,0), 1=(1,0), 2=(0,1), 3=(1,1)
            int effectiveNtID = (currentNtY * 2) + currentNtX;
            uint16_t baseAddr = 0x2000 + (effectiveNtID * 0x400);

            // --- A. Fetch Tile ---
            uint16_t tileAddr = baseAddr + (tileY * 32) + tileX;
            uint8_t tileID = bus.ppu.ppuRead(tileAddr);

            // --- B. Fetch Attribute (Color) ---
            uint16_t attrAddr = baseAddr + 0x03C0 + (tileY / 4) * 8 + (tileX / 4);
            uint8_t attrByte = bus.ppu.ppuRead(attrAddr);
            
            uint8_t shift = ((tileY & 2) << 1) | (tileX & 2);
            uint8_t paletteID = (attrByte >> shift) & 0x03;

            // --- C. Fetch Pixel ---
            // We only fetch the specific row we are drawing
            uint16_t patternAddr = bgPatternBase + (tileID * 16) + rowInTile;
            uint8_t tile_lsb = bus.ppu.ppuRead(patternAddr);
            uint8_t tile_msb = bus.ppu.ppuRead(patternAddr + 8);

            // Extract the specific column bit
            // Note: 7 - (absoluteX % 8) handles the bit ordering
            int colInTile = absoluteX % 8;
            uint8_t pixel = ((tile_lsb >> (7 - colInTile)) & 0x01) | 
                            (((tile_msb >> (7 - colInTile)) & 0x01) << 1);

            // --- D. Draw & Buffer ---
            int idx = screenY * 256 + screenX;
            
            // Lookup RGB Color
            pixelBuffer[idx] = getColorFromPaletteRam(paletteID, pixel);
            
            // Mark Priority for Sprites
            // (0 = Transparent, 1 = Opaque)
            priorityMap[idx] = (pixel != 0);
        }
    }
}
void Renderer::renderSprites(Bus& bus) {
    const auto& oam = bus.ppu.getOAM();
    uint8_t ppuctrl = bus.ppu.getControl();
    
    // Bit 3: Pattern Table for 8x8 sprites (0: $0000; 1: $1000)
    // Ignored in 8x16 mode.
    uint16_t spritePatternBase = (ppuctrl & 0x08) ? 0x1000 : 0x0000;
    
    // Bit 5: Sprite Size (0: 8x8; 1: 8x16)
    bool size_8x16 = (ppuctrl & 0x20);
    int spriteHeight = size_8x16 ? 16 : 8;

    // Iterate through 64 sprites
    // Note: NES renders sprites with lower indices ON TOP of higher indices.
    // In a painter's algorithm (drawing linearly), we must draw 63 down to 0
    // so that Sprite 0 is drawn LAST (on top).
    for (int i = 63; i >= 0; i--) {
        // OAM Layout:
        // Byte 0: Y Position (Screen Y - 1)
        // Byte 1: Tile ID
        // Byte 2: Attributes
        // Byte 3: X Position
        
        uint8_t y = oam[i*4 + 0];
        uint8_t tileID = oam[i*4 + 1];
        uint8_t attr = oam[i*4 + 2];
        uint8_t x = oam[i*4 + 3];

        // Sprite Y is "Top - 1", so we add 1.
        int spriteY = y + 1;
        
        // Hide sprites off-screen (Y >= 240 usually)
        if (spriteY >= 240) continue;

        // Attributes
        // Bit 0-1: Palette (4-7)
        uint8_t paletteID = (attr & 0x03) + 4; // Add 4 because sprites use the upper 4 palettes
        // Bit 5: Priority (0: Front, 1: Back)
        bool priorityBack = (attr & 0x20);
        // Bit 6: Flip Horizontal
        bool flipH = (attr & 0x40);
        // Bit 7: Flip Vertical
        bool flipV = (attr & 0x80);

        // Render Rows (8 or 16)
        for (int row = 0; row < spriteHeight; row++) {
            // Calculate which row of the sprite we are drawing (handling Vertical Flip)
            int currentSpriteRow = flipV ? (spriteHeight - 1 - row) : row;

            uint16_t patternAddr;
            
            if (!size_8x16) {
                // --- 8x8 Mode ---
                patternAddr = spritePatternBase + (tileID * 16) + currentSpriteRow;
            } else {
                // --- 8x16 Mode ---
                // Even tiles use $0000, Odd tiles use $1000
                uint16_t table = (tileID & 0x01) ? 0x1000 : 0x0000;
                uint8_t index = tileID & 0xFE; // Ignore bottom bit for index
                
                // Top half (rows 0-7) uses index, Bottom half (rows 8-15) uses index+1
                if (currentSpriteRow < 8) {
                    patternAddr = table + (index * 16) + currentSpriteRow;
                } else {
                    patternAddr = table + ((index + 1) * 16) + (currentSpriteRow - 8);
                }
            }

            // Fetch Bitplanes
            uint8_t tile_lsb = bus.ppu.ppuRead(patternAddr);
            uint8_t tile_msb = bus.ppu.ppuRead(patternAddr + 8);

            // Render Columns (8 pixels)
            for (int col = 0; col < 8; col++) {
                // Handle Horizontal Flip (read bits from other end)
                int currentSpriteCol = flipH ? col : (7 - col);
                
                uint8_t pixel = ((tile_lsb >> currentSpriteCol) & 0x01) | 
                                (((tile_msb >> currentSpriteCol) & 0x01) << 1);

                // If pixel is transparent (0), sprites don't draw
                if (pixel == 0) continue;

                int bufX = x + col;
                int bufY = spriteY + row;

                if (bufX < 256 && bufY < 240) {
                    int idx = bufY * 256 + bufX;

                    // Priority Check:
                    // If Priority is "Back" AND the Background Pixel is Opaque, skip drawing.
                    if (priorityBack && priorityMap[idx]) {
                        continue;
                    }

                    pixelBuffer[idx] = getColorFromPaletteRam(paletteID, pixel);
                }
            }
        }
    }
}

uint32_t Renderer::getColorFromPaletteRam(uint8_t palette, uint8_t pixel) {
    // Address Mapping:
    // Background Palette 0: $3F00-$3F03
    // Background Palette 1: $3F05-$3F07
    // ...
    
    // Base address for background palettes is $3F00.
    // Each palette is 4 bytes.
    uint16_t addr = 0x3F00 + (palette * 4) + pixel;
    
    // Transparency Rule: 
    // If pixel index is 0, it is transparent and uses the global background color at $3F00.
    if (pixel == 0) {
        addr = 0x3F00;
    }
    
    // Read index from PPU Palette RAM (masked to 6 bits)
    uint8_t nesColorIndex = paletteCache[addr & 0x001F] & 0x3F;
    
    // Lookup RGB value from system palette
    const auto& rgb = systemPalette[nesColorIndex];
    
    // Return formatted ARGB
    return (0xFF000000 | (rgb.r << 16) | (rgb.g << 8) | rgb.b);
}