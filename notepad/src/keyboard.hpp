#ifndef KEYBOARD_HPP
#define KEYBOARD_HPP

#include <cstdint>

namespace Hardware {

class Keyboard {
public:
    static char get_char(); // キーが押されるまで待機して文字を返す

private:
    static inline uint8_t inb(uint16_t port) {
        uint8_t val;
        __asm__ volatile("inb %1, %0" : "=a"(val) : "Nd"(port));
        return val;
    }
    static char scancode_to_ascii(uint8_t scancode);
};

} // namespace Hardware

#endif // KEYBOARD_HPP
