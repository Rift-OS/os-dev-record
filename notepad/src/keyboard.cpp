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

// 静的メンバ変数の実体定義
bool Keyboard::shift_pressed = false;

char Keyboard::get_char() {
    while (true) {
        // ステータスポート（0x64）の第0ビットが1になる（データがある）のを待つ
        if (inb(0x64) & 1) {
            uint8_t scancode = inb(0x60);

            // 左Shift(0x2A) または 右Shift(0x36) が押された（メイク）
            if (scancode == 0x2A || scancode == 0x36) {
                shift_pressed = true;
                continue;
            }
            // 左Shift(0xAA) または 右Shift(0xB6) が離された（ブレイク）
            if (scancode == 0xAA || scancode == 0xB6) {
                shift_pressed = false;
                continue;
            }

            // 第7ビットが立っているものはキーが離されたイベント（ブレイクコード）なので無視
            if (scancode & 0x80) {
                continue;
            }

            // キーが押された場合の処理
            char ascii = scancode_to_ascii(scancode);
            if (ascii != 0) {
                return ascii;
            }
        }
        __asm__ volatile("pause");
    }
}

char Keyboard::scancode_to_ascii(uint8_t scancode) {
    // 0x00 ~ 0x39 までの通常のメイクコードに対応するASCIIマップ
    // インデックス 0~63: 通常時 / インデックス 64~127: Shift押下時
    static const char ascii_map[128] = {
        // --- 通常入力時 (Shiftなし) ---
        0,    27,  '1',  '2',  '3',  '4',  '5',  '6',  '7',  '8',  '9',  '0',  '-',  '=',  '\b', '\t',
        'q',  'w',  'e',  'r',  't',  'y',  'u',  'i',  'o',  'p',  '[',  ']',  '\n', 0,   'a',  's',
        'd',  'f',  'g',  'h',  'j',  'k',  'l',  ';', '\'',  '`',   0,  '\\',  'z',  'x',  'c',  'v',
        'b',  'n',  'm',  ',',  '.',  '/',   0,   '*',   0,    ' ',   0,    0,    0,    0,    0,    0,

        // --- Shift同時押し時 (scancode + 64) ---
        0,    27,  '!',  '@',  '#',  '$',  '%',  '^',  '&',  '*',  '(',  ')',  '_',  '+',  '\b', '\t',
        'Q',  'W',  'E',  'R',  'T',  'Y',  'U',  'I',  'O',  'P',  '{',  '}',  '\n', 0,   'A',  'S',
        'D',  'F',  'G',  'H',  'J',  'K',  'L',  ':',  '"',  '~',   0,   '|',  'Z',  'X',  'C',  'V',
        'B',  'N',  'M',  '<',  '>',  '?',   0,   '*',   0,    ' ',   0,    0,    0,    0,    0,    0
    };

    if (scancode < 64) {
        if (shift_pressed) {
            return ascii_map[scancode + 64];
        }
        return ascii_map[scancode];
    }
    return 0;
}

} // namespace Hardware
