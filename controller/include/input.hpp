#pragma once

#include <cstdint>
#include <SDL2/SDL.h>

class Input {
    public:
        Input();
        ~Input();

        // CPU Interface ($4016)
        uint8_t read();
        void write(uint8_t data);

        void update();

    private:
        uint8_t shift_register = 0;
        uint8_t button_states = 0;
        bool strobe = false;

        SDL_GameController* controller = nullptr;

        uint8_t getControllerState();
};