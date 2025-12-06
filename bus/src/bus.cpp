#include "bus.hpp"
#include "logger.hpp"

Bus::Bus() {
    cpuRam.fill(0);
    
    testRam.resize(65536);
    cartridgeROM.reserve(32768);

    log("BUS", "Bus initialized.");
}

Bus::~Bus() = default;

uint8_t Bus::readCartridge(uint16_t address) {
    if (cartridgeROM.empty()) return 0;

    if (address < 0x8000) return 0; 

    uint32_t rom_address = address - 0x8000;
    try {
        if (rom_address < cartridgeROM.size()) {
            return cartridgeROM.at(rom_address); 
        }
    } catch (...) {
        return 0;
    }
    
    return 0;
}

void Bus::setTestMode(bool enabled) {
    testMode = enabled;
}

uint8_t Bus::read(uint16_t address) {
    if (testMode) return testRam[address];

    // 1. Internal RAM ($0000 - $1FFF)
    if (address < 0x2000) {
        return cpuRam[address & 0x07FF];
    }
    // 2. PPU Registers ($2000 - $3FFF)
    else if (address < 0x4000) {
        return ppu.cpuRead(address & 0x0007);
    }
    // 2.5 Input Registers ($4016 - $4017)
    else if (address == 0x4016) {
        return input.read();
    }
    // 3. APU & I/O ($4000 - $4017)
    else if (address < 0x4018) {
        return 0; 
    }
    // 4. Cartridge Space ($4020 - $FFFF)
    else if (address >= 0x4020) {
        return readCartridge(address);
    }
    
    return 0x00;
}

void Bus::write(uint16_t address, uint8_t data) {
    if (testMode) {
        testRam[address] = data;
        return;
    }

    if (address < 0x2000) {
        cpuRam[address & 0x07FF] = data;
    } else if (address < 0x4000) {
        ppu.cpuWrite(address & 0x0007, data);
    } else if (address == 0x4014) {
        // Writing XX copies 256 bytes from $XX00-$XXFF to OAM
        uint16_t page = static_cast<uint16_t>(data) << 8;
        
        // Perform the copy instantly
        for (int i = 0; i < 256; ++i) {
            // Read from CPU Bus (Usually RAM $0000-$07FF, but can be ROM)
            uint8_t val = read(page + i);
            ppu.writeOAMData(val);
        }
        
        // Account for CPU pause
        // DMA takes 513 or 514 cycles. We use 513 as a safe average.
        dma_cycles = 513; 
    } else if (address == 0x4016) {
        input.write(data);
    } else if (address < 0x4018) {
        // APU & I/O ($4000 - $4017)
    } else if (address >= 0x4020) {
        // Mapper writes would go here
    }
}

void Bus::loadROM(const std::vector<uint8_t>& prg, bool mirror_upper) {
    cartridgeROM = prg;
    if (mirror_upper && cartridgeROM.size() <= 16384) {
        cartridgeROM.resize(32768);
        for (size_t i = 0; i < 16384; ++i) {
            cartridgeROM[16384 + i] = cartridgeROM[i];
        }
    }
}