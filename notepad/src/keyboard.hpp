/*
 * Copyright 2026 Rift-OS Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://apache.org
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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

    // Shiftキーの押下状態を管理するフラグ
    static bool shift_pressed;
};

} // namespace Hardware

#endif // KEYBOARD_HPP
