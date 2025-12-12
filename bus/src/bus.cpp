#include "bus.hpp"
#include "logger.hpp"

Bus::Bus() {
    cpuRam.fill(0);
    // Initialize PPU and APU if needed, though their ctors handle most of it.
    log("BUS", "Bus initialized.");
}

Bus::~Bus() = default;

void Bus::insertCartridge(const std::shared_ptr<Cartridge>& cartridge) {
    this->cart = cartridge;
    ppu.connectCartridge(cartridge);
}

void Bus::reset() {
    cpuRam.fill(0);
    ppu.reset();
    apu.reset();
    if (cart) cart->reset();
    dma_cycles = 0;
}

void Bus::setTestMode(bool enabled) {
    testMode = enabled;
    if (enabled) {
        testRam.resize(65536);
        std::fill(testRam.begin(), testRam.end(), 0);
    }
}

bool Bus::getIRQ() const {
    // Combine APU IRQ with Mapper IRQ
    bool cartIRQ = cart ? cart->getIRQ() : false;
    return apu.irq_asserted || cartIRQ;
}

uint8_t Bus::read(uint16_t address) {
    if (testMode) return testRam[address];

    uint8_t data = 0x00;

    // 1. Cartridge ($4020 - $FFFF)
    if (cart && cart->cpuRead(address, data)) {
        return data;
    }
    // 2. Internal RAM ($0000 - $1FFF)
    else if (address < 0x2000) {
        return cpuRam[address & 0x07FF];
    }
    // 3. PPU Registers ($2000 - $3FFF)
    else if (address < 0x4000) {
        return ppu.cpuRead(address & 0x0007);
    }
    // 4. Controller 1 ($4016)
    else if (address == 0x4016) {
        // Only allow reading from the input object here.
        // Reading shifts the register, so we must not do it for 4017.
        return input.read();
    }
    // 5. Controller 2 ($4017)
    else if (address == 0x4017) {
        // Return 0 for unconnected second controller
        return 0x00;
    }
    // 6. APU ($4000 - $4017)
    // Note: $4015 is status, others are mostly write-only.
    else if (address < 0x4018) {
        if (address == 0x4015) {
            return apu.cpuRead(address);
        }
    }

    return data;
}

void Bus::write(uint16_t address, uint8_t data) {
    if (testMode) {
        testRam[address] = data;
        return;
    }

    if (cart && cart->cpuWrite(address, data)) {
        // Cartridge handled the write (e.g. Mapper registers)
        return;
    }
    else if (address < 0x2000) {
        cpuRam[address & 0x07FF] = data;
    } 
    else if (address < 0x4000) {
        ppu.cpuWrite(address & 0x0007, data);
    } 
    else if (address == 0x4014) {
        // OAM DMA
        uint16_t page = static_cast<uint16_t>(data) << 8;
        std::array<uint8_t, 256> pageData;
        for (int i = 0; i < 256; ++i) {
            pageData[i] = read(page + i);
        }
        ppu.startOAMDMA(pageData);
        // DMA timing is approx 513/514 cycles.
        dma_cycles = 513; 
    } 
    else if (address == 0x4016) {
        // Strobe works for both, but usually physically wired to both ports' latch lines.
        // Since we only emulate one input object, we write to it here.
        input.write(data);
    } 
    else if (address < 0x4018) {
        // APU Registers, including $4017 (Frame Counter)
        apu.cpuWrite(address, data);
    }
}