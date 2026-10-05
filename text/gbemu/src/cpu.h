#pragma once
#include <cstdint>
#include "memory.h"

class CPU {
public:
    CPU(Memory& mem);
    int step();
    void reset();

    // Registers (public for debugging)
    uint8_t a = 0, f = 0, b = 0, c = 0, d = 0, e = 0, h = 0, l = 0;
    uint16_t sp = 0, pc = 0;
    bool ime = false;

private:
    Memory& memory;

    uint8_t fetch();
    uint16_t fetch16();
    int execute(uint8_t opcode);

    // Flag helpers
    bool flagZ() const { return (f & 0x80) != 0; }
    bool flagN() const { return (f & 0x40) != 0; }
    bool flagH() const { return (f & 0x20) != 0; }
    bool flagC() const { return (f & 0x10) != 0; }
    void setZ(bool v) { v ? f |= 0x80 : f &= ~0x80; }
    void setN(bool v) { v ? f |= 0x40 : f &= ~0x40; }
    void setH(bool v) { v ? f |= 0x20 : f &= ~0x20; }
    void setC(bool v) { v ? f |= 0x10 : f &= ~0x10; }

    // 16-bit register pairs
    uint16_t getBC() const { return (b << 8) | c; }
    uint16_t getDE() const { return (d << 8) | e; }
    uint16_t getHL() const { return (h << 8) | l; }
    void setBC(uint16_t v) { b = v >> 8; c = v & 0xFF; }
    void setDE(uint16_t v) { d = v >> 8; e = v & 0xFF; }
    void setHL(uint16_t v) { h = v >> 8; l = v & 0xFF; }

    void push(uint16_t value);
    uint16_t pop();
};