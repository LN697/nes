#include "cartridge.hpp"
#include <fstream>
#include <iostream>

Cartridge::Cartridge(const std::string& sFileName) {
    struct sHeader {
        char name[4];
        uint8_t prg_rom_chunks;
        uint8_t chr_rom_chunks;
        uint8_t mapper1;
        uint8_t mapper2;
        uint8_t prg_ram_size;
        uint8_t tv_system1;
        uint8_t tv_system2;
        char unused[5];
    } header;

    bImageValid = false;

    std::ifstream ifs(sFileName, std::ifstream::binary);
    if (ifs.is_open()) {
        ifs.read((char*)&header, sizeof(sHeader));

        if (header.name[0] == 'N' && header.name[1] == 'E' && header.name[2] == 'S' && header.name[3] == 0x1A) {
            
            nPRGBanks = header.prg_rom_chunks;
            nCHRBanks = header.chr_rom_chunks;

            nMapperID = ((header.mapper2 >> 4) << 4) | (header.mapper1 >> 4);
            hwMirror = (header.mapper1 & 0x01) ? MirrorMode::VERTICAL : MirrorMode::HORIZONTAL;

            if (header.mapper1 & 0x04) {
                ifs.seekg(512, std::ios_base::cur);
            }

            // Load PRG
            vPRGMemory.resize(nPRGBanks * 16384);
            ifs.read((char*)vPRGMemory.data(), vPRGMemory.size());

            // Load CHR
            if (nCHRBanks == 0) {
                vCHRMemory.resize(8192);
            } else {
                vCHRMemory.resize(nCHRBanks * 8192);
                ifs.read((char*)vCHRMemory.data(), vCHRMemory.size());
            }
            
            // Load Mapper
            switch (nMapperID) {
                case 0: 
                    pMapper = std::make_shared<Mapper_000>(nPRGBanks, nCHRBanks); 
                    break;
                default:
                    std::cout << "Unsupported Mapper ID: " << (int)nMapperID << std::endl;
                    pMapper = std::make_shared<Mapper_000>(nPRGBanks, nCHRBanks);
                    break;
            }

            bImageValid = true;
            ifs.close();
        }
    }
}

Cartridge::~Cartridge() {}

bool Cartridge::ImageValid() { return bImageValid; }

void Cartridge::reset() {
    if (pMapper != nullptr) {
        pMapper->reset();
    }
}

bool Cartridge::cpuRead(uint16_t addr, uint8_t &data) {
    uint32_t mapped_addr = 0;
    if (pMapper->cpuMapRead(addr, mapped_addr, data)) {
        if (mapped_addr < vPRGMemory.size()) {
            data = vPRGMemory[mapped_addr];
        } else {
            data = 0;
        }
        return true;
    }
    return false;
}

bool Cartridge::cpuWrite(uint16_t addr, uint8_t data) {
    uint32_t mapped_addr = 0;
    if (pMapper->cpuMapWrite(addr, mapped_addr, data)) {
        if (mapped_addr < vPRGMemory.size()) {
            vPRGMemory[mapped_addr] = data;
        }
        return true;
    }
    return false;
}

bool Cartridge::ppuRead(uint16_t addr, uint8_t &data) {
    uint32_t mapped_addr = 0;
    if (pMapper->ppuMapRead(addr, mapped_addr)) {
        if (mapped_addr < vCHRMemory.size()) {
            data = vCHRMemory[mapped_addr];
        }
        return true;
    }
    return false;
}

bool Cartridge::ppuWrite(uint16_t addr, uint8_t data) {
    uint32_t mapped_addr = 0;
    if (pMapper->ppuMapWrite(addr, mapped_addr)) {
        if (mapped_addr < vCHRMemory.size()) {
            vCHRMemory[mapped_addr] = data;
        }
        return true;
    }
    return false;
}

MirrorMode Cartridge::getMirroring() {
    MirrorMode mode = pMapper->getMirroringMode();
    if (mode == MirrorMode::HARDWARE) {
        return hwMirror;
    }
    return mode;
}

bool Cartridge::getIRQ() {
    if (pMapper) {
        return pMapper->getIRQ();
    }
    return false;
}