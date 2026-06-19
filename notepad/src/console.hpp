#ifndef CONSOLE_HPP
#define CONSOLE_HPP

#include <cstdint>

namespace Hardware {

class Console {
public:
    static constexpr int WIDTH = 80;
    static constexpr int HEIGHT = 25;

    static void init();
    static void clear();
    static void put_char(char c, uint8_t color = 0x0A); // デフォルト黄緑色
    static void put_string(const char* str, uint8_t color = 0x0A);
    static void set_cursor(int x, int y);
    static void backspace();

private:
    static unsigned short* const video_memory;
    static int cursor_x;
    static int cursor_y;

    static void scroll();
    static void update_hardware_cursor();
};

} // namespace Hardware

#endif // CONSOLE_HPP
