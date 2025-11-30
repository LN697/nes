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
    // 256x128 buffer for two side-by-side pattern tables
    pixelBuffer.resize(256 * 240);
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
    // 1. Decode both Pattern Tables from PPU Memory
    // Table 0: $0000 - $0FFF
    // renderPatternTable(0, 0, bus); 
    // Table 1: $1000 - $1FFF
    // renderPatternTable(1, 0, bus);

    renderNameTable(0, bus); // Draw Nametable 0 for testing

    // 2. Upload to Texture
    SDL_UpdateTexture(texture, nullptr, pixelBuffer.data(), 256 * sizeof(uint32_t));

    // 3. Render
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    
    // Draw the pattern tables to the screen
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

void Renderer::renderNameTable(int ntID, Bus& bus) {
    // 1. Calculate Nametable Base Address
    // ID 0=$2000, 1=$2400, 2=$2800, 3=$2C00
    uint16_t baseAddr = 0x2000 + (ntID * 0x400);

    // 2. Determine Pattern Table for Background
    // This is normally controlled by PPUCTRL ($2000) Bit 4.
    // Since we don't have a public getter for PPUCTRL yet, we hardcode it.
    // For Super Mario Bros, the background uses Table 1 ($1000).
    // TODO: Implement bus.ppu.getControl() to replace this hardcoded value.
    uint16_t bgPatternBase = (bus.ppu.getControl() & 0x10) ? 0x1000 : 0x0000;

    // Iterate over the 32x30 tile grid (256x240 pixels)
    for (int y = 0; y < 30; y++) {
        for (int x = 0; x < 32; x++) {
            
            // --- Step A: Fetch Tile ID ---
            // The Nametable is a simple array of bytes (0-959)
            uint16_t tileAddr = baseAddr + (y * 32) + x;
            uint8_t tileID = bus.ppu.ppuRead(tileAddr);

            // --- Step B: Fetch Attribute (Color Palette) ---
            // The Attribute Table starts after the Nametable (offset $03C0).
            // Each byte controls a 4x4 tile block (32x32 pixels).
            uint16_t attrAddr = baseAddr + 0x03C0 + (y / 4) * 8 + (x / 4);
            uint8_t attrByte = bus.ppu.ppuRead(attrAddr);
            
            // A byte contains 4 palettes (2 bits each). We must find which quadrant we are in.
            // Quadrants: TopLeft(0), TopRight(2), BottomLeft(4), BottomRight(6)
            // Logic: if y%4 >= 2, we are bottom. if x%4 >= 2, we are right.
            uint8_t shift = ((y & 2) << 1) | (x & 2);
            uint8_t paletteID = (attrByte >> shift) & 0x03;

            // --- Step C: Fetch Pattern Data (Pixels) ---
            // Each tile is 16 bytes (Plane 0 + Plane 1)
            uint16_t patternAddr = bgPatternBase + (tileID * 16);

            for (int row = 0; row < 8; row++) {
                // Read the two bitplanes for this row
                uint8_t tile_lsb = bus.ppu.ppuRead(patternAddr + row);
                uint8_t tile_msb = bus.ppu.ppuRead(patternAddr + row + 8);

                for (int col = 0; col < 8; col++) {
                    // Combine bits to get pixel index (0-3)
                    // Pixels are stored MSB to LSB (Left to Right)
                    uint8_t pixel = ((tile_lsb >> (7 - col)) & 0x01) | 
                                    (((tile_msb >> (7 - col)) & 0x01) << 1);

                    // Calculate position in the output buffer
                    int bufX = (x * 8) + col;
                    int bufY = (y * 8) + row;
                    
                    // Safety Bounds Check
                    if (bufX < 256 && bufY < 240) {
                        int idx = bufY * 256 + bufX;
                        pixelBuffer[idx] = getColorFromPaletteRam(paletteID, pixel, bus);
                    }
                }
            }
        }
    }
}

// Helper to resolve the final ARGB color
uint32_t Renderer::getColorFromPaletteRam(uint8_t palette, uint8_t pixel, Bus& bus) {
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
    uint8_t nesColorIndex = bus.ppu.ppuRead(addr) & 0x3F;
    
    // Lookup RGB value from system palette
    const auto& rgb = systemPalette[nesColorIndex];
    
    // Return formatted ARGB
    return (0xFF000000 | (rgb.r << 16) | (rgb.g << 8) | rgb.b);
}