#pragma once
#include <cstdint>
#if __has_include(<SDL2/SDL.h>)
#include <SDL2/SDL.h>
#elif __has_include(<SDL.h>)
#include <SDL.h>
#else
#error "SDL2 headers not found. Add the SDL2 include directory to the compiler include paths."
#endif

const int SCREEN_WIDTH = 160;
const int SCREEN_HEIGHT = 144;

class PPU {
public:
    PPU();
    ~PPU();
    bool init();
    void render();
    bool handleEvents();
    void shutdown();

    uint8_t lcdc = 0x91;
    uint8_t stat = 0x00;
    uint8_t scy = 0, scx = 0;
    uint8_t ly = 0, lyc = 0;
    uint8_t vram[8192] = {0};
    uint8_t oam[160] = {0};

private:
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* texture = nullptr;
    uint32_t framebuffer[SCREEN_WIDTH * SCREEN_HEIGHT];
    bool running = true;
};