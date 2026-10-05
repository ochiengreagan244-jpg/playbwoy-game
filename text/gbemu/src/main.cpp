#include <iostream>
#include "memory.h"
#include "cpu.h"
#include "ppu.h"

int main(int argc, char* argv[]) {
    std::cout << "GB Emulator Starting..." << std::endl;

    Memory memory;
    CPU cpu(memory);
    PPU ppu;

    if (!ppu.init()) {
        std::cerr << "PPU init failed: " << SDL_GetError() << std::endl;
        return 1;
    }

    if (argc > 1) {
        if (!memory.loadROM(argv[1])) {
            std::cerr << "Failed to load ROM" << std::endl;
            return 1;
        }
    } else {
        // Test program: LD A, 0x42; XOR A; JP 0x0100
        memory.write(0x0100, 0x3E);
        memory.write(0x0101, 0x42);
        memory.write(0x0102, 0xAF);
        memory.write(0x0103, 0xC3);
        memory.write(0x0104, 0x00);
        memory.write(0x0105, 0x01);
    }

    std::cout << "Running. Press ESC to quit." << std::endl;

    int frame = 0;
    while (ppu.handleEvents()) {
        for (int i = 0; i < 70000; ) {
            i += cpu.step();
        }
        ppu.render();
        frame++;

        if (frame % 60 == 0) {
            std::cout << "Frame " << frame << " PC=0x" << std::hex << cpu.pc << std::dec << std::endl;
        }
    }

    return 0;
}