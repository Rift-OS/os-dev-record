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

#include "keyboard.hpp"

namespace Hardware {

char Keyboard::get_char() {
    while (true) {
        // ステータスポート（0x64）の第0ビットが1になる（データがある）のを待つ
        if (inb(0x64) & 1) {
            uint8_t scancode = inb(0x60);
            // 第7ビットが立っているものはキーが離されたイベント（ブレイクコード）なので無視
            if (!(scancode & 0x80)) {
                char ascii = scancode_to_ascii(scancode);
                if (ascii != 0) {
                    return ascii;
                }
            }
        }
        __asm__ volatile("pause");
    }
}

char Keyboard::scancode_to_ascii(uint8_t scancode) {
    //だいたい全部のやつを
    static const char ascii_map[128] = {
        0,    27,  '1',  '2',  '3',  '4',  '5',  '6',  '7',  '8',  '9',  '0',  '-',  '=',  '\b', '\t',
        'q',  'w',  'e',  'r',  't',  'y',  'u',  'i',  'o',  'p',  '[',  ']',  '\n', 0,   'a',  's',
        'd',  'f',  'g',  'h',  'j',  'k',  'l',  ';', '\'',  '`',   0,  '\\',  'z',  'x',  'c',  'v',
        'b',  'n',  'm',  ',',  '.',  '/',   0,   '*',   0,    ' ',   0,    0,    0,    0,    0,    0,
         0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    '-',  0,    0,    0,    '+',  0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        '!',  '@',  '#',  '$',  '%',  '^',  '&',  '*',  '(',  ')',  '_',  '+',  '\b', '\t', 'Q',  'W',
        'E',  'R',  'T',  'Y',  'U',  'I',  'O',  'P',  '{',  '}',  '\n', 0,   'A',  'S',  'D',  'F'
    };
    if (scancode < 128) {
        return ascii_map[scancode];
    }
    return 0;
}

} // namespace Hardware
