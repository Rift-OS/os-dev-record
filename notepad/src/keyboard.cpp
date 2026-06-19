#include "keyboard.hpp"

namespace Hardware {

char Keyboard::get_char() {
    while (true) {
        // キーボードコントローラのステータスポート(0x64)を確認
        uint8_t status = inb(0x64);
        if (status & 0x01) { // 出力バッファにデータ
