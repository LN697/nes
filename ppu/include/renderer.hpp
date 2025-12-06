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

    bool init(const char* title, int width, int height, int scale);
    
    // Main Render Loop function (Call this once per frame)
    void draw(Bus& bus);
    
    bool handleEvents();

private:
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* texture = nullptr;

    struct RGB { uint8_t r, g, b; };
    static const std::array<RGB, 64> systemPalette;

    std::vector<uint32_t> pixelBuffer;
    std::array<uint8_t, 32> paletteCache;

    std::vector<bool> priorityMap;

    void renderPatternTable(int tableIndex, uint8_t paletteID, Bus& bus);
    void renderNameTable(Bus& bus);
    void renderSprites(Bus& bus);
    
    uint32_t getColor(uint8_t index);
    uint32_t getColorFromPaletteRam(uint8_t palette, uint8_t pixel);
};
