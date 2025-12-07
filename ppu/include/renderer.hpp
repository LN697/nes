#pragma once

#include <SDL2/SDL.h>
#include <vector>
#include <cstdint>

class Renderer {
    public:
        Renderer();
        ~Renderer();

        bool init(const char* title, int width, int height, int scale);
        void draw(const std::vector<uint32_t>& buffer);
        bool handleEvents();

    private:
        SDL_Window* window = nullptr;
        SDL_Renderer* renderer = nullptr;
        SDL_Texture* texture = nullptr;
        SDL_Event event;
};