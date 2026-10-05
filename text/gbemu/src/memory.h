#pragma once
#include <cstdint>
#include <vector>
#include <string>

class Memory {
public:
    Memory();
    bool loadROM(const std::string& path);
    uint8_t read(uint16_t address);
    void write(uint16_t address, uint8_t value);
    uint16_t read16(uint16_t address);
    void write16(uint16_t address, uint16_t value);

    // Timer registers
    uint8_t div = 0;
    uint8_t tima = 0;
    uint8_t tma = 0;
    uint8_t tac = 0;

private:
    std::vector<uint8_t> ram;
    std::vector<uint8_t> rom;
};