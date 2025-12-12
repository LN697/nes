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
    
    void writeControl(uint8_t data);    // $4000 / $4004
    void writeSweep(uint8_t data);      // $4001 / $4005
    void writeTimerLow(uint8_t data);   // $4002 / $4006
    void writeTimerHigh(uint8_t data);  // $4003 / $4007
    void setEnabled(bool e);
    
    void stepTimer();
    void stepEnvelope();
    void stepLength();
    
    uint8_t getOutput();
    bool isEnabled() const { return enabled; }

public:
    bool enabled = false;
    
    uint16_t timer_period = 0;
    uint16_t timer_value = 0;
    
    uint8_t duty_mode = 0;
    uint8_t duty_pos = 0;
    static const uint8_t duty_table[4][8];

    bool constant_volume = false;
    bool env_loop = false;
    bool env_start_flag = false;
    uint8_t vol_period = 0;
    uint8_t env_divider = 0;
    uint8_t decay_level = 0;

    uint8_t length_counter = 0;
    static const uint8_t length_table[32];
};

// =============================================================
// TRIANGLE CHANNEL
// =============================================================
class TriangleChannel {
public:
    TriangleChannel();

    void writeLinearCounter(uint8_t data); // $4008
    void writeTimerLow(uint8_t data);      // $400A
    void writeTimerHigh(uint8_t data);     // $400B
    void setEnabled(bool e);

    void stepTimer();
    void stepLength();
    void stepLinearCounter();

    uint8_t getOutput();

public:
    bool enabled = false;

    // Timer
    uint16_t timer_period = 0;
    uint16_t timer_value = 0;

    // Sequencer (32-step)
    uint8_t seq_pos = 0;
    static const uint8_t sequence_table[32];

    // Linear Counter
    bool lc_control_flag = false; // Also halts length counter
    bool lc_reload_flag = false;
    uint8_t lc_reload_value = 0;
    uint8_t linear_counter = 0;

    // Length Counter
    uint8_t length_counter = 0;
};

// =============================================================
// NOISE CHANNEL
// =============================================================
class NoiseChannel {
public:
    NoiseChannel();

    void writeControl(uint8_t data);    // $400C
    void writeMode(uint8_t data);       // $400E
    void writeLength(uint8_t data);     // $400F
    void setEnabled(bool e);

    void stepTimer();
    void stepEnvelope();
    void stepLength();

    uint8_t getOutput();

public:
    bool enabled = false;

    // Timer
    uint16_t timer_period = 0;
    uint16_t timer_value = 0;
    static const uint16_t period_table[16];

    // LFSR
    uint16_t lfsr = 1;
    bool mode_flag = false; // Mode 0: 32767 steps, Mode 1: 93 steps

    // Envelope
    bool constant_volume = false;
    bool env_loop = false; // Also halts length counter
    bool env_start_flag = false;
    uint8_t vol_period = 0;
    uint8_t env_divider = 0;
    uint8_t decay_level = 0;

    // Length Counter
    uint8_t length_counter = 0;
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
    TriangleChannel triangle;
    NoiseChannel noise;

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

    // Variables for mixing
    uint8_t p1_out, p2_out, tri_out, noise_out, dmc_out;
    float sample_out, pulse_out, tnd_out;
};