#include "audio.hpp"
#include <algorithm>
#include <iostream>
#include <cmath>

// =============================================================
// PULSE CHANNEL IMPLEMENTATION
// =============================================================

const uint8_t PulseChannel::duty_table[4][8] = {
    {0, 1, 0, 0, 0, 0, 0, 0}, // 12.5%
    {0, 1, 1, 0, 0, 0, 0, 0}, // 25%
    {0, 1, 1, 1, 1, 0, 0, 0}, // 50%
    {1, 0, 0, 1, 1, 1, 1, 1}  // 75% (Negated 25%)
};

const uint8_t PulseChannel::length_table[32] = {
    10, 254, 20, 2, 40, 4, 80, 6, 160, 8, 60, 10, 14, 12, 26, 14, 
    12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30
};

PulseChannel::PulseChannel() {
    // Power-up state
}

void PulseChannel::setEnabled(bool e) {
    enabled = e;
    if (!enabled) {
        length_counter = 0;
    }
}

void PulseChannel::writeControl(uint8_t data) {
    // $4000 / $4004
    // DDLC VVVV
    duty_mode = (data >> 6) & 0x03;
    env_loop = (data & 0x20) != 0;      // Also Halt Length Counter
    constant_volume = (data & 0x10) != 0;
    vol_period = data & 0x0F;
}

void PulseChannel::writeSweep(uint8_t data) {
    // $4001 / $4005
    // EPPP NSSS (Enabled, Period, Negate, Shift)
    // TODO: Implement sweep logic
    (void)data;
}

void PulseChannel::writeTimerLow(uint8_t data) {
    // $4002 / $4006
    // LLLL LLLL
    timer_period = (timer_period & 0x0700) | data;
}

void PulseChannel::writeTimerHigh(uint8_t data) {
    // $4003 / $4007
    // LLLL LHHH
    timer_period = (timer_period & 0x00FF) | (static_cast<uint16_t>(data & 0x07) << 8);
    
    // Side effect: Reset duty sequencer and start envelope
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
        timer_value = (timer_period + 1) * 2; // Reload
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
            env_divider = vol_period + 1; // Reload
            
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
    if (timer_period < 8) return 0; // Hardware Sweep Mute behavior

    // Check Duty Cycle
    if (duty_table[duty_mode][duty_pos] == 0) return 0;

    // Return Volume
    if (constant_volume) {
        return vol_period;
    } else {
        return decay_level;
    }
}

// =============================================================
// APU IMPLEMENTATION
// =============================================================

APU::APU() {
    // Initialize SDL Audio
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
        std::cerr << "APU: SDL Audio init failed: " << SDL_GetError() << std::endl;
    } else {
        SDL_AudioSpec want, have;
        SDL_zero(want);
        want.freq = AUDIO_SAMPLE_RATE;
        want.format = AUDIO_F32;
        want.channels = 1;
        want.samples = 1024; // Hardware Buffer size
        
        audio_device = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
        
        if (audio_device == 0) {
            std::cerr << "APU: Failed to open audio device: " << SDL_GetError() << std::endl;
        } else {
            SDL_PauseAudioDevice(audio_device, 0); // Unpause
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

        // Status
        case 0x4015: 
            pulse1.setEnabled((data & 0x01) != 0);
            pulse2.setEnabled((data & 0x02) != 0);
            break;

        // Frame Counter
        case 0x4017:
            frame_mode = (data & 0x80) >> 7;
            irq_inhibit = (data & 0x40) != 0;
            if (irq_inhibit) irq_asserted = false;
            
            frame_clock_counter = 0;
            
            if (frame_mode == 1) {
                pulse1.stepEnvelope(); pulse2.stepEnvelope();
                pulse1.stepLength();   pulse2.stepLength();
            }
            break;
    }
}

uint8_t APU::cpuRead(uint16_t addr) {
    if (addr == 0x4015) {
        // Status Register
        uint8_t data = 0x00;
        if (pulse1.length_counter > 0) data |= 0x01;
        if (pulse2.length_counter > 0) data |= 0x02;
        if (irq_asserted) data |= 0x40;
        
        irq_asserted = false; // Reading clears Frame IRQ
        return data;
    }
    return 0;
}

void APU::step(int cycles) {
    for (int i = 0; i < cycles; ++i) {
        if (frame_clock_counter % 2 == 0) {
            pulse1.stepTimer();
            pulse2.stepTimer();
        }

        frame_clock_counter++;
        stepFrameCounter();

        time_accumulator++;
        if (time_accumulator >= time_per_sample) {
            time_accumulator -= time_per_sample;
            generateSample();
        }
    }
}

void APU::stepFrameCounter() {
    // Mode 0: 4-Step
    // Mode 1: 5-Step
    
    if (frame_mode == 0) {
        if (frame_clock_counter == 7457) {
            pulse1.stepEnvelope(); pulse2.stepEnvelope();
        } else if (frame_clock_counter == 14915) {
            pulse1.stepEnvelope(); pulse2.stepEnvelope();
            pulse1.stepLength();   pulse2.stepLength();
        } else if (frame_clock_counter == 22372) {
            pulse1.stepEnvelope(); pulse2.stepEnvelope();
        } else if (frame_clock_counter == 29829) {
            pulse1.stepEnvelope(); pulse2.stepEnvelope();
            pulse1.stepLength();   pulse2.stepLength();
            if (!irq_inhibit) irq_asserted = true;
        } else if (frame_clock_counter == 29830) {
            if (!irq_inhibit) irq_asserted = true;
            frame_clock_counter = 0;
        }
    } else {
        // Mode 1 (5-Step)
        if (frame_clock_counter == 7457) {
            pulse1.stepEnvelope(); pulse2.stepEnvelope();
        } else if (frame_clock_counter == 14915) {
            pulse1.stepEnvelope(); pulse2.stepEnvelope();
            pulse1.stepLength();   pulse2.stepLength();
        } else if (frame_clock_counter == 22372) {
            pulse1.stepEnvelope(); pulse2.stepEnvelope();
        } else if (frame_clock_counter == 29829) {
            // Do nothing
        } else if (frame_clock_counter == 37281) {
            pulse1.stepEnvelope(); pulse2.stepEnvelope();
            pulse1.stepLength();   pulse2.stepLength();
        } else if (frame_clock_counter == 37282) {
            frame_clock_counter = 0;
        }
    }
}

void APU::generateSample() {
    if (audio_device == 0) return;

    p1 = pulse1.getOutput();
    p2 = pulse2.getOutput();
    
    pulse_out = 0.0f;
    if (p1 > 0 || p2 > 0) {
        pulse_out = 95.88f / ((8128.0f / (p1 + p2)) + 100.0f);
    }
    
    sample = pulse_out;
    SDL_QueueAudio(audio_device, &sample, sizeof(float));
}