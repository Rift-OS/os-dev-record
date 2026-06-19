#include "console.hpp"

namespace Hardware {

unsigned short* const Console::video_memory = (unsigned short*)0xb8000;
int Console::cursor_x = 0;
int Console::cursor_y = 0;

void Console::init() {
    clear();
}

void Console::clear() {
    for (int i = 0; i < WIDTH * HEIGHT; i++) {
        video_memory[i] = (unsigned short)' ' | (0x07 << 8);
    }
    cursor_x = 0;
    cursor_y = 0;
    update_hardware_cursor();
}

void Console::put_char(char c, uint8_t color) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else if (c == '\r') {
        cursor_x = 0;
    } else {
        int index = cursor_y * WIDTH + cursor_x;
        video_memory[index] = (unsigned short)c | (color << 8);
        cursor_x++;
    }

    if (cursor_x >= WIDTH) {
        cursor_x = 0;
        cursor_y++;
    }

    scroll();
    update_hardware_cursor();
}

void Console::put_string(const char* str, uint8_t color) {
    for (int i = 0; str[i] != '\0'; i++) {
        put_char(str[i], color);
    }
}

void Console::backspace() {
    if (cursor_x > 0) {
        cursor_x--;
    } else if (cursor_y > 0) {
        cursor_y--;
        cursor_x = WIDTH - 1;
    }
    int index = cursor_y * WIDTH + cursor_x;
    video_memory[index] = (unsigned short)' ' | (0x07 << 8);
    update_hardware_cursor();
}

void Console::scroll() {
    if (cursor_y >= HEIGHT) {
        for (int i = 0; i < WIDTH * (HEIGHT - 1); i++) {
            video_memory[i] = video_memory[i + WIDTH];
        }
        for (int i = WIDTH * (HEIGHT - 1); i < WIDTH * HEIGHT; i++) {
            video_memory[i] = (unsigned short)' ' | (0x07 << 8);
        }
        cursor_y = HEIGHT - 1;
    }
}

void Console::update_hardware_cursor() {
    uint16_t position = cursor_y * WIDTH + cursor_x;
    // CRTコントローラレジスタを叩いてカーソル位置を同期
    __asm__ volatile("outb %0, %1" :: "a"((uint8_t)0x0F), "Nd"((uint16_t)0x3D4));
    __asm__ volatile("outb %0, %1" :: "a"((uint8_t)(position & 0xFF)), "Nd"((uint16_t)0x3D5));
    __asm__ volatile("outb %0, %1" :: "a"((uint8_t)0x0E), "Nd"((uint16_t)0x3D4));
    __asm__ volatile("outb %0, %1" :: "a"((uint8_t)((position >> 8) & 0xFF)), "Nd"((uint16_t)0x3D5));
}

} // namespace Hardware
