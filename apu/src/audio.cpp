#include "audio.hpp"
#include <algorithm>
#include <iostream>
#include <cmath>

// =============================================================
// SHARED LOOKUP TABLES
// =============================================================

// Shared length table for Pulse and Noise (and Triangle uses the same mapping logic)
// Indices based on the 5-bit length index (llll l)
const uint8_t PulseChannel::length_table[32] = {
    10, 254, 20, 2, 40, 4, 80, 6, 160, 8, 60, 10, 14, 12, 26, 14, 
    12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30
};

// =============================================================
// PULSE CHANNEL IMPLEMENTATION
// =============================================================

const uint8_t PulseChannel::duty_table[4][8] = {
    {0, 1, 0, 0, 0, 0, 0, 0}, // 12.5%
    {0, 1, 1, 0, 0, 0, 0, 0}, // 25%
    {0, 1, 1, 1, 1, 0, 0, 0}, // 50%
    {1, 0, 0, 1, 1, 1, 1, 1}  // 75% (Negated 25%)
};

PulseChannel::PulseChannel() {}

void PulseChannel::setEnabled(bool e) {
    enabled = e;
    if (!enabled) length_counter = 0;
}

void PulseChannel::writeControl(uint8_t data) {
    // $4000 / $4004: DDLC VVVV
    duty_mode = (data >> 6) & 0x03;
    env_loop = (data & 0x20) != 0;      // Also Halt Length Counter
    constant_volume = (data & 0x10) != 0;
    vol_period = data & 0x0F;
}

void PulseChannel::writeSweep(uint8_t data) {
    // $4001 / $4005: EPPP NSSS (Sweep functionality placeholder)
    (void)data;
}

void PulseChannel::writeTimerLow(uint8_t data) {
    // $4002 / $4006: LLLL LLLL
    timer_period = (timer_period & 0x0700) | data;
}

void PulseChannel::writeTimerHigh(uint8_t data) {
    // $4003 / $4007: LLLL LHHH
    timer_period = (timer_period & 0x00FF) | (static_cast<uint16_t>(data & 0x07) << 8);
    
    if (enabled) {
        length_counter = length_table[(data >> 3) & 0x1F];
    }
    
    duty_pos = 0;
    env_start_flag = true;
}

void PulseChannel::stepTimer() {
    if (timer_value > 0) {
        timer_value--;
    } else {
        timer_value = (timer_period + 1) * 2; // Pulse channels clock every 2 CPU cycles
        duty_pos = (duty_pos + 1) & 0x07;
    }
}

void PulseChannel::stepEnvelope() {
    if (env_start_flag) {
        env_start_flag = false;
        decay_level = 15;
        env_divider = vol_period + 1;
    } else {
        if (env_divider > 0) {
            env_divider--;
        } else {
            env_divider = vol_period + 1;
            if (decay_level > 0) {
                decay_level--;
            } else if (env_loop) {
                decay_level = 15;
            }
        }
    }
}

void PulseChannel::stepLength() {
    if (!env_loop && length_counter > 0) {
        length_counter--;
    }
}

uint8_t PulseChannel::getOutput() {
    if (!enabled) return 0;
    if (length_counter == 0) return 0;
    if (timer_period < 8) return 0; // Sweep mute limit

    if (duty_table[duty_mode][duty_pos] == 0) return 0;

    return constant_volume ? vol_period : decay_level;
}

// =============================================================
// TRIANGLE CHANNEL IMPLEMENTATION
// =============================================================

const uint8_t TriangleChannel::sequence_table[32] = {
    15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
};

TriangleChannel::TriangleChannel() {}

void TriangleChannel::setEnabled(bool e) {
    enabled = e;
    if (!enabled) length_counter = 0;
}

void TriangleChannel::writeLinearCounter(uint8_t data) {
    // $4008: CRRR RRRR (Control/Halt, Reload Value)
    lc_control_flag = (data & 0x80) != 0;
    lc_reload_value = data & 0x7F;
}

void TriangleChannel::writeTimerLow(uint8_t data) {
    // $400A: LLLL LLLL
    timer_period = (timer_period & 0xFF00) | data;
}

void TriangleChannel::writeTimerHigh(uint8_t data) {
    // $400B: LLLL LHHH
    timer_period = (timer_period & 0x00FF) | (static_cast<uint16_t>(data & 0x07) << 8);
    
    if (enabled) {
        length_counter = PulseChannel::length_table[(data >> 3) & 0x1F];
    }
    lc_reload_flag = true;
}

void TriangleChannel::stepTimer() {
    if (timer_value > 0) {
        timer_value--;
    } else {
        timer_value = timer_period; // Triangle timer is CPU Frequency (no *2 multiplier like Pulse)
        // Timer clocks the sequencer only if linear/length counters are active
        if (length_counter > 0 && linear_counter > 0) {
             seq_pos = (seq_pos + 1) & 0x1F;
        }
    }
}

void TriangleChannel::stepLength() {
    if (!lc_control_flag && length_counter > 0) {
        length_counter--;
    }
}

void TriangleChannel::stepLinearCounter() {
    if (lc_reload_flag) {
        linear_counter = lc_reload_value;
    } else if (linear_counter > 0) {
        linear_counter--;
    }
    
    if (!lc_control_flag) {
        lc_reload_flag = false;
    }
}

uint8_t TriangleChannel::getOutput() {
    if (!enabled) return 0;
    // Note: The triangle channel does not silence on length/linear = 0, 
    // it simply halts the sequencer. If it halts on a high value, it holds that value.
    // However, for typical emulation, getting the value from the table is sufficient.
    return sequence_table[seq_pos];
}

// =============================================================
// NOISE CHANNEL IMPLEMENTATION
// =============================================================

const uint16_t NoiseChannel::period_table[16] = {
    4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068
};

NoiseChannel::NoiseChannel() {}

void NoiseChannel::setEnabled(bool e) {
    enabled = e;
    if (!enabled) length_counter = 0;
}

void NoiseChannel::writeControl(uint8_t data) {
    // $400C: --LC VVVV
    env_loop = (data & 0x20) != 0;
    constant_volume = (data & 0x10) != 0;
    vol_period = data & 0x0F;
}

void NoiseChannel::writeMode(uint8_t data) {
    // $400E: M--- PPPP
    mode_flag = (data & 0x80) != 0;
    timer_period = period_table[data & 0x0F];
}

void NoiseChannel::writeLength(uint8_t data) {
    // $400F: LLLL L---
    if (enabled) {
        length_counter = PulseChannel::length_table[(data >> 3) & 0x1F];
    }
    env_start_flag = true;
}

void NoiseChannel::stepTimer() {
    if (timer_value > 0) {
        timer_value--;
    } else {
        timer_value = timer_period;
        
        // Shift LFSR
        uint8_t feedback_bit_pos = mode_flag ? 6 : 1;
        uint16_t feedback = (lfsr & 0x01) ^ ((lfsr >> feedback_bit_pos) & 0x01);
        lfsr >>= 1;
        lfsr |= (feedback << 14);
    }
}

void NoiseChannel::stepEnvelope() {
    if (env_start_flag) {
        env_start_flag = false;
        decay_level = 15;
        env_divider = vol_period + 1;
    } else {
        if (env_divider > 0) {
            env_divider--;
        } else {
            env_divider = vol_period + 1;
            if (decay_level > 0) {
                decay_level--;
            } else if (env_loop) {
                decay_level = 15;
            }
        }
    }
}

void NoiseChannel::stepLength() {
    if (!env_loop && length_counter > 0) {
        length_counter--;
    }
}

uint8_t NoiseChannel::getOutput() {
    if (!enabled) return 0;
    if (length_counter == 0) return 0;
    if ((lfsr & 0x01) == 1) return 0; // Bit 0 is 1 means silence

    return constant_volume ? vol_period : decay_level;
}


// =============================================================
// APU IMPLEMENTATION
// =============================================================

APU::APU() {
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
        std::cerr << "APU: SDL Audio init failed: " << SDL_GetError() << std::endl;
    } else {
        SDL_AudioSpec want, have;
        SDL_zero(want);
        want.freq = AUDIO_SAMPLE_RATE;
        want.format = AUDIO_F32;
        want.channels = 1;
        want.samples = 1024;
        
        audio_device = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
        
        if (audio_device == 0) {
            std::cerr << "APU: Failed to open audio device: " << SDL_GetError() << std::endl;
        } else {
            SDL_PauseAudioDevice(audio_device, 0);
            std::cout << "APU: Audio initialized at " << have.freq << "Hz" << std::endl;
        }
    }

    time_per_sample = CPU_FREQUENCY / AUDIO_SAMPLE_RATE;
    reset();
}

APU::~APU() {
    if (audio_device) {
        SDL_CloseAudioDevice(audio_device);
    }
}

void APU::reset() {
    cpuWrite(0x4015, 0x00);
    cpuWrite(0x4017, 0x00);
    irq_asserted = false;
    time_accumulator = 0;
}

void APU::cpuWrite(uint16_t addr, uint8_t data) {
    switch (addr) {
        // Pulse 1
        case 0x4000: pulse1.writeControl(data); break;
        case 0x4001: pulse1.writeSweep(data); break;
        case 0x4002: pulse1.writeTimerLow(data); break;
        case 0x4003: pulse1.writeTimerHigh(data); break;
        
        // Pulse 2
        case 0x4004: pulse2.writeControl(data); break;
        case 0x4005: pulse2.writeSweep(data); break;
        case 0x4006: pulse2.writeTimerLow(data); break;
        case 0x4007: pulse2.writeTimerHigh(data); break;
        
        // Triangle
        case 0x4008: triangle.writeLinearCounter(data); break;
        case 0x4009: break; // Unused
        case 0x400A: triangle.writeTimerLow(data); break;
        case 0x400B: triangle.writeTimerHigh(data); break;

        // Noise
        case 0x400C: noise.writeControl(data); break;
        case 0x400D: break; // Unused
        case 0x400E: noise.writeMode(data); break;
        case 0x400F: noise.writeLength(data); break;

        // Status
        case 0x4015: 
            pulse1.setEnabled((data & 0x01) != 0);
            pulse2.setEnabled((data & 0x02) != 0);
            triangle.setEnabled((data & 0x04) != 0);
            noise.setEnabled((data & 0x08) != 0);
            // DMC enabled = (data & 0x10) != 0;
            break;

        // Frame Counter
        case 0x4017:
            frame_mode = (data & 0x80) >> 7;
            irq_inhibit = (data & 0x40) != 0;
            if (irq_inhibit) irq_asserted = false;
            
            frame_clock_counter = 0;
            
            if (frame_mode == 1) {
                pulse1.stepEnvelope(); pulse2.stepEnvelope(); noise.stepEnvelope();
                pulse1.stepLength();   pulse2.stepLength();   noise.stepLength();
                triangle.stepLength(); triangle.stepLinearCounter();
            }
            break;
    }
}

uint8_t APU::cpuRead(uint16_t addr) {
    if (addr == 0x4015) {
        uint8_t data = 0x00;
        if (pulse1.length_counter > 0) data |= 0x01;
        if (pulse2.length_counter > 0) data |= 0x02;
        if (triangle.length_counter > 0) data |= 0x04;
        if (noise.length_counter > 0) data |= 0x08;
        // DMC check...
        
        if (irq_asserted) data |= 0x40;
        irq_asserted = false;
        return data;
    }
    return 0;
}

void APU::step(int cycles) {
    for (int i = 0; i < cycles; ++i) {
        // Clock timers
        if (frame_clock_counter % 2 == 0) {
            pulse1.stepTimer();
            pulse2.stepTimer();
            noise.stepTimer();
            // DMC timer...
        }
        triangle.stepTimer();

        // Frame Counter
        frame_clock_counter++;
        stepFrameCounter();

        // Audio Sampling
        time_accumulator++;
        if (time_accumulator >= time_per_sample) {
            time_accumulator -= time_per_sample;
            generateSample();
        }
    }
}

void APU::stepFrameCounter() {
    // Mode 0: 4-step sequence
    // Mode 1: 5-step sequence
    
    bool quarter_frame = false;
    bool half_frame = false;

    if (frame_mode == 0) {
        if (frame_clock_counter == 7457)   quarter_frame = true;
        if (frame_clock_counter == 14915)  { quarter_frame = true; half_frame = true; }
        if (frame_clock_counter == 22372)  quarter_frame = true;
        if (frame_clock_counter == 29829)  { quarter_frame = true; half_frame = true; if (!irq_inhibit) irq_asserted = true; }
        if (frame_clock_counter == 29830)  { if (!irq_inhibit) irq_asserted = true; frame_clock_counter = 0; }
    } else {
        if (frame_clock_counter == 7457)   quarter_frame = true;
        if (frame_clock_counter == 14915)  { quarter_frame = true; half_frame = true; }
        if (frame_clock_counter == 22372)  quarter_frame = true;
        if (frame_clock_counter == 37281)  { quarter_frame = true; half_frame = true; }
        if (frame_clock_counter == 37282)  frame_clock_counter = 0;
    }

    if (quarter_frame) {
        pulse1.stepEnvelope();
        pulse2.stepEnvelope();
        noise.stepEnvelope();
        triangle.stepLinearCounter();
    }

    if (half_frame) {
        pulse1.stepLength();
        pulse2.stepLength();
        noise.stepLength();
        triangle.stepLength();
    }
}

void APU::generateSample() {
    if (audio_device == 0) return;

    p1_out = pulse1.getOutput();
    p2_out = pulse2.getOutput();
    tri_out = triangle.getOutput();
    noise_out = noise.getOutput();
    dmc_out = 0; // Not implemented yet
    
    // --- MIXER ---
    // Pulse Mix
    pulse_out = 0.0f;
    if (p1_out > 0 || p2_out > 0) {
        pulse_out = 95.88f / ((8128.0f / (p1_out + p2_out)) + 100.0f);
    }
    
    // TND (Triangle, Noise, DMC) Mix
    // Formula: 159.79 / (1 / (triangle / 8227 + noise / 12241 + dmc / 22638) + 100)
    tnd_out = 0.0f;
    if (tri_out > 0 || noise_out > 0 || dmc_out > 0) {
        tnd_out = 159.79f / ((1.0f / ((tri_out / 8227.0f) + (noise_out / 12241.0f) + (dmc_out / 22638.0f))) + 100.0f);
    }
    
    sample_out = pulse_out + tnd_out;
    
    SDL_QueueAudio(audio_device, &sample_out, sizeof(float));
}