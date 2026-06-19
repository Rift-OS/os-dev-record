#include "console.hpp"
#include "keyboard.hpp"

extern "C" void kernel_main() {
    // 画面の初期化
    Hardware::Console::init();
    
    // メインタイトルバーの描画 (背景白、文字黒の最上段バー)
    Hardware::Console::put_string("--- Rift OS notepad v1 (Type anything / Backspace available) ---\n", 0xF0);
    
    // 入力ループ
    while (1) {
        // キーボードから1文字取得
        char c = Hardware::Keyboard::get_char();
        
        if (c == '\b') {
            // バックスペースキーが押されたら1文字消す
            Hardware::Console::backspace();
        } else {
            // それ以外の通常文字や改行はそのまま画面に出力 (黄緑色)
            Hardware::Console::put_char(c, 0x0A);
        }
    }
}
