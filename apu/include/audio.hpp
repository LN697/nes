#pragma once

#include <cstdint>
#include <vector>
#include <array>
#include <SDL2/SDL.h>

// =============================================================
// PULSE CHANNEL
// =============================================================
class PulseChannel {
public:
    PulseChannel();
    
    // Registers
    void writeControl(uint8_t data);    // $4000 / $4004
    void writeSweep(uint8_t data);      // $4001 / $4005
    void writeTimerLow(uint8_t data);   // $4002 / $4006
    void writeTimerHigh(uint8_t data);  // $4003 / $4007
    void setEnabled(bool e);
    
    // Components
    void stepTimer();
    void stepEnvelope();
    void stepLength();
    
    uint8_t getOutput();
    bool isEnabled() const { return enabled; }

public: // Made public for debug/viewing
    bool enabled = false;
    
    // Timer
    uint16_t timer_period = 0;
    uint16_t timer_value = 0;
    
    // Duty Cycle
    uint8_t duty_mode = 0;
    uint8_t duty_pos = 0;
    static const uint8_t duty_table[4][8];

    // Envelope
    bool constant_volume = false;
    bool env_loop = false;
    bool env_start_flag = false;
    uint8_t vol_period = 0;     // Also decay rate
    uint8_t env_divider = 0;
    uint8_t decay_level = 0;

    // Length Counter
    uint8_t length_counter = 0;
    static const uint8_t length_table[32];
};

// =============================================================
// APU
// =============================================================
class APU {
public:
    APU();
    ~APU();

    void reset();

    // CPU Interface
    void cpuWrite(uint16_t addr, uint8_t data);
    uint8_t cpuRead(uint16_t addr);

    // Clocking
    void step(int cycles); // Runs at CPU frequency

    // Interrupts
    bool irq_asserted = false;

private:
    void stepFrameCounter();
    void generateSample();

    // Channels
    PulseChannel pulse1;
    PulseChannel pulse2;

    // Frame Counter
    uint64_t frame_clock_counter = 0;
    uint8_t frame_mode = 0; // 0: 4-step, 1: 5-step
    bool irq_inhibit = false;

    // SDL2 Audio
    SDL_AudioDeviceID audio_device = 0;
    double time_per_sample = 0.0;
    double time_accumulator = 0.0;
    
    // Constants
    static constexpr int AUDIO_SAMPLE_RATE = 44100;
    static constexpr double CPU_FREQUENCY = 1789773.0;

    // Variables
    uint8_t p1, p2;
    float pulse_out, sample;
};