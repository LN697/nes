#include "memory.hpp"
#include "logger.hpp"
#include "core.hpp"
#include "register.hpp"
#include "core_policies.hpp" // Include for Op_ADC and Op_ASL
#include <sstream>
#include <stdexcept>

// Helper to print register state
void printRegisterState(const std::string& regName, const Register& reg) {
    log("TEST", "Register " + regName + ": Value = " + std::to_string(reg.getValue()) + ", Size = " + std::to_string(reg.getSize()) + " bits");
}

// Helper to print core flags
void printCoreFlags(const Core& core) {
    std::string flags = "P Flags: ";
    flags += core.getStatusFlag(Core::StatusFlag::N) ? "N" : ".";
    flags += core.getStatusFlag(Core::StatusFlag::V) ? "V" : ".";
    flags += "."; // Unused bit 5
    flags += core.getStatusFlag(Core::StatusFlag::B) ? "B" : ".";
    // The Decimal Flag (D) is ignored in NES 6502, so it's not checked here.
    flags += core.getStatusFlag(Core::StatusFlag::I) ? "I" : ".";
    flags += core.getStatusFlag(Core::StatusFlag::Z) ? "Z" : ".";
    flags += core.getStatusFlag(Core::StatusFlag::C) ? "C" : ".";
    log("TEST", flags);
}

int main() {
    log("MAIN", "Configuring the CPU emulator for NES.");

    Memory mem(64 * KB); // 64KB memory
    // Initialize memory for some basic operations if needed
    mem.setMemory(0x0000, 0x00); // Example initial value for PC

    Core core(&mem);

    log("TEST", "--- Register Size and Exception Tests ---");
    // Test 8-bit register (e.g., A, X, Y, S, P)
    try {
        log("TEST", "Testing 8-bit register 'a'");
        core.a.setValue(255); // Should be fine
        printRegisterState("A", core.a);
        core.a.setValue(256); // Should throw an exception
        printRegisterState("A", core.a); // This line should not be reached
    } catch (const std::out_of_range& e) {
        log("TEST", "Caught expected exception for A: " + std::string(e.what()));
    }
    core.a.setValue(0); // Reset for next tests

    // Test 16-bit register (e.g., PC)
    try {
        log("TEST", "Testing 16-bit register 'pc'");
        core.pc.setValue(0xFFFF); // Should be fine
        printRegisterState("PC", core.pc);
        core.pc.setValue(0x10000); // Should throw an exception
        printRegisterState("PC", core.pc); // This line should not be reached
    } catch (const std::out_of_range& e) {
        log("TEST", "Caught expected exception for PC: " + std::string(e.what()));
    }
    core.pc.setValue(0); // Reset for next tests

    log("TEST", "--- ADC Instruction Tests ---");
    // Test ADC without carry
    core.a.setValue(0x10);
    core.setStatusFlag(Core::StatusFlag::C, false);
    core.setStatusFlag(Core::StatusFlag::V, false);
    core.setStatusFlag(Core::StatusFlag::N, false);
    core.setStatusFlag(Core::StatusFlag::Z, false);
    log("TEST", "ADC: A = 0x10, M = 0x05, C = 0");
    Op_ADC::exec(core, 0x05); // A = 0x10 + 0x05 + 0 = 0x15
    printRegisterState("A", core.a);
    printCoreFlags(core);
    if (core.a.getValue() == 0x15 && !core.getStatusFlag(Core::StatusFlag::C) && !core.getStatusFlag(Core::StatusFlag::V) && !core.getStatusFlag(Core::StatusFlag::Z) && !core.getStatusFlag(Core::StatusFlag::N)) {
        log("TEST", "ADC Test 1 (0x10 + 0x05): PASSED");
    } else {
        log("TEST", "ADC Test 1 (0x10 + 0x05): FAILED");
    }

    // Test ADC with carry
    core.a.setValue(0x10);
    core.setStatusFlag(Core::StatusFlag::C, true);
    core.setStatusFlag(Core::StatusFlag::V, false);
    core.setStatusFlag(Core::StatusFlag::N, false);
    core.setStatusFlag(Core::StatusFlag::Z, false);
    log("TEST", "ADC: A = 0x10, M = 0x05, C = 1");
    Op_ADC::exec(core, 0x05); // A = 0x10 + 0x05 + 1 = 0x16
    printRegisterState("A", core.a);
    printCoreFlags(core);
    if (core.a.getValue() == 0x16 && !core.getStatusFlag(Core::StatusFlag::C) && !core.getStatusFlag(Core::StatusFlag::V) && !core.getStatusFlag(Core::StatusFlag::Z) && !core.getStatusFlag(Core::StatusFlag::N)) {
        log("TEST", "ADC Test 2 (0x10 + 0x05 + C): PASSED");
    } else {
        log("TEST", "ADC Test 2 (0x10 + 0x05 + C): FAILED");
    }

    // Test ADC with overflow (positive to negative)
    core.a.setValue(0x50); // 80 decimal
    core.setStatusFlag(Core::StatusFlag::C, false);
    core.setStatusFlag(Core::StatusFlag::V, false);
    core.setStatusFlag(Core::StatusFlag::N, false);
    core.setStatusFlag(Core::StatusFlag::Z, false);
    log("TEST", "ADC: A = 0x50, M = 0x50, C = 0 (Overflow Test)");
    Op_ADC::exec(core, 0x50); // 0x50 + 0x50 = 0xA0 (160 decimal, negative flag set, overflow set)
    printRegisterState("A", core.a);
    printCoreFlags(core);
    if (core.a.getValue() == 0xA0 && !core.getStatusFlag(Core::StatusFlag::C) && core.getStatusFlag(Core::StatusFlag::V) && !core.getStatusFlag(Core::StatusFlag::Z) && core.getStatusFlag(Core::StatusFlag::N)) {
        log("TEST", "ADC Test 3 (0x50 + 0x50, Overflow): PASSED");
    } else {
        log("TEST", "ADC Test 3 (0x50 + 0x50, Overflow): FAILED");
    }

    // Test ADC with carry and negative result
    core.a.setValue(0x80); // -128 in signed 8-bit
    core.setStatusFlag(Core::StatusFlag::C, false);
    core.setStatusFlag(Core::StatusFlag::V, false);
    core.setStatusFlag(Core::StatusFlag::N, false);
    core.setStatusFlag(Core::StatusFlag::Z, false);
    log("TEST", "ADC: A = 0x80, M = 0x01, C = 0 (Negative Result Test)");
    Op_ADC::exec(core, 0x01); // 0x80 + 0x01 = 0x81 (negative)
    printRegisterState("A", core.a);
    printCoreFlags(core);
    if (core.a.getValue() == 0x81 && !core.getStatusFlag(Core::StatusFlag::C) && !core.getStatusFlag(Core::StatusFlag::V) && !core.getStatusFlag(Core::StatusFlag::Z) && core.getStatusFlag(Core::StatusFlag::N)) {
        log("TEST", "ADC Test 4 (0x80 + 0x01, Negative): PASSED");
    } else {
        log("TEST", "ADC Test 4 (0x80 + 0x01, Negative): FAILED");
    }

    log("TEST", "--- ASL Instruction Tests ---");
    // Test ASL without carry
    core.a.setValue(0x10); // 0001 0000
    core.setStatusFlag(Core::StatusFlag::C, false);
    core.setStatusFlag(Core::StatusFlag::N, false);
    core.setStatusFlag(Core::StatusFlag::Z, false);
    log("TEST", "ASL: A = 0x10");
    uint8_t asl_res1 = Op_ASL::calc(core, static_cast<uint8_t>(core.a.getValue())); // Should be 0x20
    core.a.setValue(asl_res1);
    printRegisterState("A", core.a);
    printCoreFlags(core);
    if (core.a.getValue() == 0x20 && !core.getStatusFlag(Core::StatusFlag::C) && !core.getStatusFlag(Core::StatusFlag::Z) && !core.getStatusFlag(Core::StatusFlag::N)) {
        log("TEST", "ASL Test 1 (0x10): PASSED");
    } else {
        log("TEST", "ASL Test 1 (0x10): FAILED");
    }

    // Test ASL with carry
    core.a.setValue(0xC0); // 1100 0000
    core.setStatusFlag(Core::StatusFlag::C, false);
    core.setStatusFlag(Core::StatusFlag::N, false);
    core.setStatusFlag(Core::StatusFlag::Z, false);
    log("TEST", "ASL: A = 0xC0");
    uint8_t asl_res2 = Op_ASL::calc(core, static_cast<uint8_t>(core.a.getValue())); // Should be 0x80, carry set
    core.a.setValue(asl_res2);
    printRegisterState("A", core.a);
    printCoreFlags(core);
    if (core.a.getValue() == 0x80 && core.getStatusFlag(Core::StatusFlag::C) && !core.getStatusFlag(Core::StatusFlag::Z) && core.getStatusFlag(Core::StatusFlag::N)) {
        log("TEST", "ASL Test 2 (0xC0, Carry): PASSED");
    } else {
        log("TEST", "ASL Test 2 (0xC0, Carry): FAILED");
    }

    // Test ASL to zero
    core.a.setValue(0x80); // 1000 0000
    core.setStatusFlag(Core::StatusFlag::C, false);
    core.setStatusFlag(Core::StatusFlag::N, false);
    core.setStatusFlag(Core::StatusFlag::Z, false);
    log("TEST", "ASL: A = 0x80 (to zero)");
    uint8_t asl_res3 = Op_ASL::calc(core, static_cast<uint8_t>(core.a.getValue())); // Should be 0x00, carry set
    core.a.setValue(asl_res3);
    printRegisterState("A", core.a);
    printCoreFlags(core);
    if (core.a.getValue() == 0x00 && core.getStatusFlag(Core::StatusFlag::C) && core.getStatusFlag(Core::StatusFlag::Z) && !core.getStatusFlag(Core::StatusFlag::N)) {
        log("TEST", "ASL Test 3 (0x80, Zero): PASSED");
    } else {
        log("TEST", "ASL Test 3 (0x80, Zero): FAILED");
    }

    core.run(); // Original run, might not be necessary after extensive unit tests

    log("CORE", "Core ID: " + std::to_string(core.core_id));
    
    return 0;
}