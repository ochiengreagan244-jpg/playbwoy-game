#include "cpu.h"
#include <iostream>

CPU::CPU(Memory& mem) : memory(mem) { reset(); }

void CPU::reset() {
    a = 0x01; f = 0xB0;
    b = 0x00; c = 0x13;
    d = 0x00; e = 0xD8;
    h = 0x01; l = 0x4D;
    sp = 0xFFFE;
    pc = 0x0100;
    ime = false;
    halted = false;
}

uint8_t CPU::fetch() {
    return memory.read(pc++);
}

uint16_t CPU::fetch16() {
    uint16_t lo = fetch();
    uint16_t hi = fetch();
    return lo | (hi << 8);
}

void CPU::push(uint16_t value) {
    sp--;
    memory.write(sp, value >> 8);
    sp--;
    memory.write(sp, value & 0xFF);
}

uint16_t CPU::pop() {
    uint16_t lo = memory.read(sp++);
    uint16_t hi = memory.read(sp++);
    return lo | (hi << 8);
}

// ==================================================================
//  Register access by 3-bit code
//  Code: 0=B, 1=C, 2=D, 3=E, 4=H, 5=L, 6=(HL), 7=A
// ==================================================================
uint8_t CPU::getReg8(int code) {
    switch (code) {
        case 0: return b;
        case 1: return c;
        case 2: return d;
        case 3: return e;
        case 4: return h;
        case 5: return l;
        case 6: return memory.read(getHL());
        case 7: return a;
    }
    return 0;
}

void CPU::setReg8(int code, uint8_t value) {
    switch (code) {
        case 0: b = value; break;
        case 1: c = value; break;
        case 2: d = value; break;
        case 3: e = value; break;
        case 4: h = value; break;
        case 5: l = value; break;
        case 6: memory.write(getHL(), value); break;
        case 7: a = value; break;
    }
}

// ==================================================================
//  ALU Operations
// ==================================================================
void CPU::aluAdd(uint8_t value, bool withCarry) {
    int carry = withCarry && flagC() ? 1 : 0;
    int result = a + value + carry;

    setH(((a & 0xF) + (value & 0xF) + carry) > 0xF);
    setC(result > 0xFF);
    setN(false);
    a = result & 0xFF;
    setZ(a == 0);
}

void CPU::aluSub(uint8_t value, bool withCarry) {
    int carry = withCarry && flagC() ? 1 : 0;
    int result = a - value - carry;

    setH(((a & 0xF) - (value & 0xF) - carry) < 0);
    setC(result < 0);
    setN(true);
    a = result & 0xFF;
    setZ(a == 0);
}

void CPU::aluAnd(uint8_t value) {
    a &= value;
    setZ(a == 0); setN(false); setH(true); setC(false);
}

void CPU::aluOr(uint8_t value) {
    a |= value;
    setZ(a == 0); setN(false); setH(false); setC(false);
}

void CPU::aluXor(uint8_t value) {
    a ^= value;
    setZ(a == 0); setN(false); setH(false); setC(false);
}

void CPU::aluCp(uint8_t value) {
    // CP is SUB without storing result
    int result = a - value;
    setH(((a & 0xF) - (value & 0xF)) < 0);
    setC(result < 0);
    setN(true);
    setZ((result & 0xFF) == 0);
}

// ==================================================================
//  INC / DEC for 8-bit values
// ==================================================================
uint8_t CPU::inc8(uint8_t value) {
    uint8_t result = value + 1;
    setH((value & 0xF) == 0xF);
    setN(false);
    setZ(result == 0);
    return result;
}

uint8_t CPU::dec8(uint8_t value) {
    uint8_t result = value - 1;
    setH((value & 0xF) == 0);
    setN(true);
    setZ(result == 0);
    return result;
}

// ==================================================================
//  16-bit ADD HL, rr
// ==================================================================
void CPU::addHL(uint16_t value) {
    uint16_t hl = getHL();
    int result = hl + value;
    setN(false);
    setH(((hl & 0xFFF) + (value & 0xFFF)) > 0xFFF);
    setC(result > 0xFFFF);
    setHL(result & 0xFFFF);
}

// ==================================================================
//  Main step
// ==================================================================
int CPU::step() {
    // Handle interrupts
    uint8_t ie = memory.read(0xFFFF);
    uint8_t iflag = memory.read(0xFF0F);
    uint8_t pending = ie & iflag;

    if (halted && pending) halted = false;

    if (ime && pending) {
        ime = false;
        if (pending & 0x01) { // VBlank
            memory.write(0xFF0F, iflag & ~0x01);
            push(pc);
            pc = 0x40;
        } else if (pending & 0x02) { // STAT
            memory.write(0xFF0F, iflag & ~0x02);
            push(pc);
            pc = 0x48;
        } else if (pending & 0x04) { // Timer
            memory.write(0xFF0F, iflag & ~0x04);
            push(pc);
            pc = 0x50;
        } else if (pending & 0x08) { // Serial
            memory.write(0xFF0F, iflag & ~0x08);
            push(pc);
            pc = 0x58;
        } else if (pending & 0x10) { // Joypad
            memory.write(0xFF0F, iflag & ~0x10);
            push(pc);
            pc = 0x60;
        }
        return 20;
    }

    if (halted) return 4;

    uint8_t opcode = fetch();
    return execute(opcode);
}

// ==================================================================
//  Main opcode dispatcher
// ==================================================================
int CPU::execute(uint8_t opcode) {
    // If this is a CB-prefixed instruction, handle separately
    if (opcode == 0xCB) {
        uint8_t cb = fetch();
        return executeCB(cb);
    }

    // ----- 0x40-0x7F and 0x80-0xBF are LD r,r' and ALU A,r -----
    // We decode them algorithmically to save code.
    if (opcode >= 0x40 && opcode <= 0x7F && opcode != 0x76) {
        int dst = (opcode >> 3) & 0x07;
        int src = opcode & 0x07;
        uint8_t value = getReg8(src);
        setReg8(dst, value);
        return (src == 6 || dst == 6) ? 8 : 4; // (HL) access takes longer
    }

    if (opcode >= 0x80 && opcode <= 0xBF) {
        int op = (opcode >> 3) & 0x07;
        int src = opcode & 0x07;
        uint8_t value = getReg8(src);
        switch (op) {
            case 0: aluAdd(value); break;
            case 1: aluAdd(value, true); break;
            case 2: aluSub(value); break;
            case 3: aluSub(value, true); break;
            case 4: aluAnd(value); break;
            case 5: aluXor(value); break;
            case 6: aluOr(value); break;
            case 7: aluCp(value); break;
        }
        return (src == 6) ? 8 : 4;
    }

    // ----- Other instructions -----
    switch (opcode) {
        case 0x00: return 4; // NOP

        // LD rr, nn  (16-bit immediate loads)
        case 0x01: setBC(fetch16()); return 12;
        case 0x11: setDE(fetch16()); return 12;
        case 0x21: setHL(fetch16()); return 12;
        case 0x31: sp = fetch16(); return 12;

        // LD (rr), A
        case 0x02: memory.write(getBC(), a); return 8;
        case 0x12: memory.write(getDE(), a); return 8;
        case 0x22: memory.write(getHL(), a); setHL(getHL() + 1); return 8;
        case 0x32: memory.write(getHL(), a); setHL(getHL() - 1); return 8;

        // LD A, (rr)
        case 0x0A: a = memory.read(getBC()); return 8;
        case 0x1A: a = memory.read(getDE()); return 8;
        case 0x2A: a = memory.read(getHL()); setHL(getHL() + 1); return 8;
        case 0x3A: a = memory.read(getHL()); setHL(getHL() - 1); return 8;

        // LD r, n  (8-bit immediate)
        case 0x06: b = fetch(); return 8;
        case 0x0E: c = fetch(); return 8;
        case 0x16: d = fetch(); return 8;
        case 0x1E: e = fetch(); return 8;
        case 0x26: h = fetch(); return 8;
        case 0x2E: l = fetch(); return 8;
        case 0x36: memory.write(getHL(), fetch()); return 12;
        case 0x3E: a = fetch(); return 8;

        // INC rr
        case 0x03: setBC(getBC() + 1); return 8;
        case 0x13: setDE(getDE() + 1); return 8;
        case 0x23: setHL(getHL() + 1); return 8;
        case 0x33: sp++; return 8;

        // DEC rr
        case 0x0B: setBC(getBC() - 1); return 8;
        case 0x1B: setDE(getDE() - 1); return 8;
        case 0x2B: setHL(getHL() - 1); return 8;
        case 0x3B: sp--; return 8;

        // INC r
        case 0x04: b = inc8(b); return 4;
        case 0x0C: c = inc8(c); return 4;
        case 0x14: d = inc8(d); return 4;
        case 0x1C: e = inc8(e); return 4;
        case 0x24: h = inc8(h); return 4;
        case 0x2C: l = inc8(l); return 4;
        case 0x34: memory.write(getHL(), inc8(memory.read(getHL()))); return 12;
        case 0x3C: a = inc8(a); return 4;

        // DEC r
        case 0x05: b = dec8(b); return 4;
        case 0x0D: c = dec8(c); return 4;
        case 0x15: d = dec8(d); return 4;
        case 0x1D: e = dec8(e); return 4;
        case 0x25: h = dec8(h); return 4;
        case 0x2D: l = dec8(l); return 4;
        case 0x35: memory.write(getHL(), dec8(memory.read(getHL()))); return 12;
        case 0x3D: a = dec8(a); return 4;

        // ADD HL, rr
        case 0x09: addHL(getBC()); return 8;
        case 0x19: addHL(getDE()); return 8;
        case 0x29: addHL(getHL()); return 8;
        case 0x39: addHL(sp); return 8;

        // JR e (relative jump)
        case 0x18: {
            int8_t offset = (int8_t)fetch();
            pc += offset;
            return 12;
        }
        // JR cc, e
        case 0x20: { int8_t o = (int8_t)fetch(); if (!flagZ()) { pc += o; return 12; } return 8; }
        case 0x28: { int8_t o = (int8_t)fetch(); if ( flagZ()) { pc += o; return 12; } return 8; }
        case 0x30: { int8_t o = (int8_t)fetch(); if (!flagC()) { pc += o; return 12; } return 8; }
        case 0x38: { int8_t o = (int8_t)fetch(); if ( flagC()) { pc += o; return 12; } return 8; }

        // JP nn (absolute jump)
        case 0xC3: pc = fetch16(); return 16;
        // JP cc, nn
        case 0xC2: { uint16_t n = fetch16(); if (!flagZ()) { pc = n; return 16; } return 12; }
        case 0xCA: { uint16_t n = fetch16(); if ( flagZ()) { pc = n; return 16; } return 12; }
        case 0xD2: { uint16_t n = fetch16(); if (!flagC()) { pc = n; return 16; } return 12; }
        case 0xDA: { uint16_t n = fetch16(); if ( flagC()) { pc = n; return 16; } return 12; }

        // JP (HL)
        case 0xE9: pc = getHL(); return 4;

        // CALL nn
        case 0xCD: { uint16_t n = fetch16(); push(pc); pc = n; return 24; }
        // CALL cc, nn
        case 0xC4: { uint16_t n = fetch16(); if (!flagZ()) { push(pc); pc = n; return 24; } return 12; }
        case 0xCC: { uint16_t n = fetch16(); if ( flagZ()) { push(pc); pc = n; return 24; } return 12; }
        case 0xD4: { uint16_t n = fetch16(); if (!flagC()) { push(pc); pc = n; return 24; } return 12; }
        case 0xDC: { uint16_t n = fetch16(); if ( flagC()) { push(pc); pc = n; return 24; } return 12; }

        // RET
        case 0xC9: pc = pop(); return 16;
        // RET cc
        case 0xC0: if (!flagZ()) { pc = pop(); return 20; } return 8;
        case 0xC8: if ( flagZ()) { pc = pop(); return 20; } return 8;
        case 0xD0: if (!flagC()) { pc = pop(); return 20; } return 8;
        case 0xD8: if ( flagC()) { pc = pop(); return 20; } return 8;

        // RETI (return from interrupt, re-enable)
        case 0xD9: pc = pop(); ime = true; return 16;

        // RST (restart - call to fixed address)
        case 0xC7: push(pc); pc = 0x00; return 16;
        case 0xCF: push(pc); pc = 0x08; return 16;
        case 0xD7: push(pc); pc = 0x10; return 16;
        case 0xDF: push(pc); pc = 0x18; return 16;
        case 0xE7: push(pc); pc = 0x20; return 16;
        case 0xEF: push(pc); pc = 0x28; return 16;
        case 0xF7: push(pc); pc = 0x30; return 16;
        case 0xFF: push(pc); pc = 0x38; return 16;

        // PUSH / POP
        case 0xC5: push(getBC()); return 16;
        case 0xD5: push(getDE()); return 16;
        case 0xE5: push(getHL()); return 16;
        case 0xF5: push(getAF()); return 16;
        case 0xC1: setBC(pop()); return 12;
        case 0xD1: setDE(pop()); return 12;
        case 0xE1: setHL(pop()); return 12;
        case 0xF1: setAF(pop()); return 12;

        // ALU with immediate
        case 0xC6: aluAdd(fetch()); return 8;
        case 0xCE: aluAdd(fetch(), true); return 8;
        case 0xD6: aluSub(fetch()); return 8;
        case 0xDE: aluSub(fetch(), true); return 8;
        case 0xE6: aluAnd(fetch()); return 8;
        case 0xEE: aluXor(fetch()); return 8;
        case 0xF6: aluOr(fetch()); return 8;
        case 0xFE: aluCp(fetch()); return 8;

        // LD (nn), SP
        case 0x08: {
            uint16_t addr = fetch16();
            memory.write(addr, sp & 0xFF);
            memory.write(addr + 1, sp >> 8);
            return 20;
        }

        // Misc
        case 0x76: halted = true; return 4; // HALT
        case 0xF3: ime = false; return 4;   // DI
        case 0xFB: ime = true; return 4;    // EI
        case 0x27: /* DAA - skipped for now */ return 4;
        case 0x2F: a = ~a; setN(true); setH(true); return 4; // CPL
        case 0x37: setN(false); setH(false); setC(true); return 4; // SCF
        case 0x3F: setN(false); setH(false); setC(!flagC()); return 4; // CCF
        case 0x07: { uint8_t cy = (a >> 7) & 1; a = (a << 1) | cy; setZ(false); setN(false); setH(false); setC(cy); return 4; } // RLCA
        case 0x0F: { uint8_t cy = a & 1; a = (a >> 1) | (cy << 7); setZ(false); setN(false); setH(false); setC(cy); return 4; } // RRCA
        case 0x17: { uint8_t cy = (a >> 7) & 1; a = (a << 1) | (flagC() ? 1 : 0); setZ(false); setN(false); setH(false); setC(cy); return 4; } // RLA
        case 0x1F: { uint8_t cy = a & 1; a = (a >> 1) | (flagC() ? 0x80 : 0); setZ(false); setN(false); setH(false); setC(cy); return 4; } // RRA

        default:
            std::cerr << "Unknown opcode 0x" << std::hex << (int)opcode
                      << " at PC=0x" << (pc - 1) << std::dec << std::endl;
            return 4;
    }
}

// ==================================================================
//  CB-prefixed instructions (bit ops, rotates, shifts)
//  Format: 0xCB XX where XX = 00_yyy_zzz
//    yyy = operation, zzz = register
// ==================================================================
int CPU::executeCB(uint8_t opcode) {
    int reg = opcode & 0x07;
    int op = (opcode >> 3) & 0x07;
    int bit = (opcode >> 6) & 0x03;
    uint8_t value = getReg8(reg);
    int cycles = (reg == 6) ? 16 : 8;

    switch (bit) {
        case 0: { // Rotate/shift ops
            uint8_t result = value;
            uint8_t cy = 0;
            switch (op) {
                case 0: cy = (value >> 7) & 1; result = (value << 1) | cy; break; // RLC
                case 1: cy = value & 1; result = (value >> 1) | (cy << 7); break; // RRC
                case 2: cy = (value >> 7) & 1; result = (value << 1) | (flagC() ? 1 : 0); break; // RL
                case 3: cy = value & 1; result = (value >> 1) | (flagC() ? 0x80 : 0); break; // RR
                case 4: cy = (value >> 7) & 1; result = value << 1; break; // SLA
                case 5: cy = value & 1; result = (value >> 1) | (value & 0x80); break; // SRA
                case 6: cy = 0; result = (value << 4) | (value >> 4); break; // SWAP
                case 7: cy = value & 1; result = value >> 1; break; // SRL
            }
            setReg8(reg, result);
            setZ(result == 0); setN(false); setH(false); setC(cy);
            break;
        }
        case 1: { // BIT b, r
            int b = op;
            setZ((value & (1 << b)) == 0);
            setN(false); setH(true);
            break;
        }
        case 2: { // RES b, r (clear bit)
            int b = op;
            setReg8(reg, value & ~(1 << b));
            break;
        }
        case 3: { // SET b, r
            int b = op;
            setReg8(reg, value | (1 << b));
            break;
        }
    }
    return cycles;
}