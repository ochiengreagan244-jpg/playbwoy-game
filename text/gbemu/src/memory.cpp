#include "memory.h"
#include <fstream>
#include <iostream>

Memory::Memory() : ram(65536, 0) {}

bool Memory::loadROM(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return false;

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    rom.resize(size);
    if (!file.read(reinterpret_cast<char*>(rom.data()), size)) return false;

    for (size_t i = 0; i < rom.size() && i < 0x8000; i++) {
        ram[i] = rom[i];
    }
    return true;
}

uint8_t Memory::read(uint16_t address) {
    if (address == 0xFF04) return div;
    if (address == 0xFF05) return tima;
    if (address == 0xFF06) return tma;
    if (address == 0xFF07) return tac;
    return ram[address];
}

void Memory::write(uint16_t address, uint8_t value) {
    if (address == 0xFF04) { div = 0; return; }
    if (address == 0xFF05) { tima = value; return; }
    if (address == 0xFF06) { tma = value; return; }
    if (address == 0xFF07) { tac = value; return; }
    ram[address] = value;
}

uint16_t Memory::read16(uint16_t address) {
    return read(address) | (read(address + 1) << 8);
}

void Memory::write16(uint16_t address, uint16_t value) {
    write(address, value & 0xFF);
    write(address + 1, (value >> 8) & 0xFF);
}