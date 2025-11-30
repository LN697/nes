#pragma once

#include <SDL2/SDL.h>
#include <vector>
#include <cstdint>
#include <array>

#include "bus.hpp"

class Renderer {
public:
    Renderer();
    ~Renderer();

    // Initialize SDL Window and Renderer
    bool init(const char* title, int width, int height, int scale);
    
    // Main Render Loop function (Call this once per frame)
    void draw(Bus& bus);
    
    // Handle SDL Events (Quit, Keys)
    bool handleEvents();

private:
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* texture = nullptr;

    // The NES Hardware Palette (64 Colors)
    struct RGB { uint8_t r, g, b; };
    static const std::array<RGB, 64> systemPalette;

    // Pixel Buffer
    std::vector<uint32_t> pixelBuffer;

    // Helper: Decodes a single 8x8 tile from PPU memory
    void renderPatternTable(int tableIndex, uint8_t paletteID, Bus& bus);
    void renderNameTable(int ntID, Bus& bus);
    
    // Helper: Get color from system palette
    uint32_t getColor(uint8_t index);
    uint32_t getColorFromPaletteRam(uint8_t palette, uint8_t pixel, Bus& bus);

};