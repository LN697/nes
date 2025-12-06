#include "core.hpp"

namespace {
    static const std::string a_name("Accumulator");
    static const std::string x_name("X index");
    static const std::string y_name("Y index");
    static const std::string s_name("Stack pointer");
    static const std::string pc_name("Program Counter");
    static const std::string p_name("Processor flags");
}

Core::Core(Bus* bus_ptr)
    : bus(bus_ptr)
    , a(RegType::MR, &a_name, 8)
    , x(RegType::IR, &x_name, 8)
    , y(RegType::IR, &y_name, 8)
    , s(RegType::IR, &s_name, 8)
    , pc(RegType::PC, &pc_name, 16)
    , p(RegType::SR, &p_name, 8)
{
    core_phase = Phase::STANDBY;
    log("CORE", "Core initialized.");
}

Core::~Core() = default;

void Core::init() {
    // Reset Vector is at $FFFC
    uint16_t lo = bus->read(0xFFFC);
    uint16_t hi = bus->read(0xFFFD);
    pc.setValue((hi << 8) | lo);
    
    // Reset State
    s.setValue(0xFD);
    p.setValue(0x34); // IRQ disabled
    
    log("CORE", "Reset complete. PC: " + std::to_string(pc.getValue()));
}

void Core::step() {
    if (core_phase == Phase::ERROR) return;

    // NMI is edge-triggered. It happens once when the PPU pulls the NMI line low.
    if (bus->ppu.nmiOccurred) {
        bus->ppu.nmiOccurred = false;

        // --- NMI HARDWARE SEQUENCE (7 Cycles) ---
        uint16_t pc_val = static_cast<uint16_t>(pc.getValue());
        uint8_t sp = static_cast<uint8_t>(s.getValue());
        
        bus->write(0x0100 | sp, (pc_val >> 8) & 0xFF);
        s.setValue(sp - 1);
        sp--;

        bus->write(0x0100 | sp, pc_val & 0xFF);
        s.setValue(sp - 1);
        sp--;

        // CRITICAL: For Hardware Interrupts (NMI/IRQ), the B-Flag (Bit 4) is CLEARED on the stack.
        // Bit 5 (Unused) is always SET.
        uint8_t status = static_cast<uint8_t>(p.getValue());
        status &= ~0x10;
        status |= 0x20;
        
        bus->write(0x0100 | sp, status);
        s.setValue(sp - 1);
        
        // The CPU ignores further IRQs inside an interrupt handler.
        // (NMIs can still interrupt an NMI, technically, but PPU logic prevents this loop)
        setStatusFlag(StatusFlag::I, true);
        
        uint16_t lo = bus->read(0xFFFA);
        uint16_t hi = bus->read(0xFFFB);
        pc.setValue((hi << 8) | lo);

        // NMI takes 7 cycles.
        last_cycles = 7; 

        // log("CORE", "NMI Triggered. PC set to: " + std::to_string(pc.getValue()));
        
        return; 
    }

    uint8_t opcode = fetch();
    //log("CORE", "Opcode: " + std::to_string(opcode));

    if (instr_table[opcode]) {
        instr_table[opcode](*this);
    } else {
        log("CORE", "Unimplemented Opcode: " + std::to_string(opcode));
        core_phase = Phase::ERROR;
    }
}

std::uint8_t Core::read(std::uint16_t address) const {
    return bus->read(address);
}

void Core::write(std::uint16_t address, std::uint8_t value) {
    bus->write(address, value);
}

std::uint8_t Core::fetch() {
    std::uint8_t data = read(static_cast<std::uint16_t>(pc.getValue()));
    pc.setValue(pc.getValue() + 1);
    return data;
}

std::uint16_t Core::fetchWord() {
    std::uint16_t low = fetch();
    std::uint16_t high = fetch();
    return static_cast<std::uint16_t>((high << 8) | low);
}

void Core::setStatusFlag(StatusFlag flag, bool value) {
    if (value) p.setBit(static_cast<int>(flag));
    else p.clearBit(static_cast<int>(flag));
}

bool Core::getStatusFlag(StatusFlag flag) const {
    return p.getBit(static_cast<int>(flag));
}