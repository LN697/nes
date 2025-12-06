#include "input.hpp"
#include "logger.hpp"

#include <iostream>

Input::Input() {
    if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) < 0) {
        std::cerr << "Input Warning: SDL GameController init failed: " << SDL_GetError() << "\n";
    } else {
        // TODO: handle hot-plugging events in the main loop.
        if (SDL_NumJoysticks() > 0) {
            controller = SDL_GameControllerOpen(0);
            if (controller) {
                log("Input", "Controller connected: " + std::string(SDL_GameControllerName(controller)));
            } else {
                std::cerr << "Input: Could not open game controller: " << SDL_GetError() << "\n";
            }
        }
    }
}

Input::~Input() {
    if (controller) {
        SDL_GameControllerClose(controller);
        log("Input", "Controller disconnected");
    }
}

uint8_t Input::read() {
    uint8_t data = 0;
    if (strobe) {
        data = button_states & 1;
    } else {
        data = shift_register & 1;
        shift_register >>= 1;
        shift_register |= 0x80; 
    }
    return data;
}

void Input::write(uint8_t data) {
    bool new_strobe = (data & 1) != 0;
    if (strobe && !new_strobe) {
        shift_register = button_states;
    }
    strobe = new_strobe;
}

void Input::update() {
    button_states = getControllerState();
}

uint8_t Input::getControllerState() {
    const Uint8* keys = SDL_GetKeyboardState(nullptr);
    
        // Helper lambda to check Key OR Button OR Axis
    auto check = [&](int scancode, SDL_GameControllerButton btn) -> bool {
        // Keyboard Check
        if (keys[scancode]) return true;
        
        // Controller Check
        if (controller) {
            if (SDL_GameControllerGetButton(controller, btn)) return true;
        }
        return false;
    };

    // Helper for Analog Stick (Threshold 16000 ~= 50%)
    auto checkAxis = [&](SDL_GameControllerAxis axis, int sign) -> bool {
        if (!controller) return false;
        int16_t val = SDL_GameControllerGetAxis(controller, axis);
        if (sign > 0 && val > 16000) return true;
        if (sign < 0 && val < -16000) return true;
        return false;
    };

    uint8_t buttons = 0;

    // --- MAPPING ---
    
    // A Button (NES A) -> Keyboard Z || Controller 'A' (Bottom) or 'B' (Right)
    if (check(SDL_SCANCODE_Z, SDL_CONTROLLER_BUTTON_A) || 
        (controller && SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_B))) 
        buttons |= (1 << 0);

    // B Button (NES B) -> Keyboard X || Controller 'X' (Left) or 'Y' (Top)
    if (check(SDL_SCANCODE_X, SDL_CONTROLLER_BUTTON_X) || 
        (controller && SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_Y)))
        buttons |= (1 << 1);

    // Select -> Right Shift || Back
    if (check(SDL_SCANCODE_RSHIFT, SDL_CONTROLLER_BUTTON_BACK)) buttons |= (1 << 2);

    // Start -> Return || Start
    if (check(SDL_SCANCODE_RETURN, SDL_CONTROLLER_BUTTON_START)) buttons |= (1 << 3);

    // Up -> Arrow Up || D-Pad Up || Left Stick Up
    if (check(SDL_SCANCODE_UP, SDL_CONTROLLER_BUTTON_DPAD_UP) || checkAxis(SDL_CONTROLLER_AXIS_LEFTY, -1))     
        buttons |= (1 << 4);

    // Down -> Arrow Down || D-Pad Down || Left Stick Down
    if (check(SDL_SCANCODE_DOWN, SDL_CONTROLLER_BUTTON_DPAD_DOWN) || checkAxis(SDL_CONTROLLER_AXIS_LEFTY, 1))   
        buttons |= (1 << 5);

    // Left -> Arrow Left || D-Pad Left || Left Stick Left
    if (check(SDL_SCANCODE_LEFT, SDL_CONTROLLER_BUTTON_DPAD_LEFT) || checkAxis(SDL_CONTROLLER_AXIS_LEFTX, -1))   
        buttons |= (1 << 6);

    // Right -> Arrow Right || D-Pad Right || Left Stick Right
    if (check(SDL_SCANCODE_RIGHT, SDL_CONTROLLER_BUTTON_DPAD_RIGHT) || checkAxis(SDL_CONTROLLER_AXIS_LEFTX, 1))  
        buttons |= (1 << 7);

    return buttons;
}