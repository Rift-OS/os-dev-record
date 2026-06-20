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
