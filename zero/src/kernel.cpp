extern "C" void kernel_main() {
    // 画面（VGAバッファ）の物理アドレス
    auto *video_memory = reinterpret_cast<unsigned short *>(0xb8000);
    
    // 表示したい文字列
    const char *str = "Hello, C++ World!";
    
    // i番目の文字が '\0' になるまで繰り返す
    for (int i = 0; str[i] != '\0'; i++) {
        // 文字を書き込む（0x0a は黄緑色）
        video_memory[i] = static_cast<unsigned short>(str[i]) | (0x0a << 8);
    }
    
    // CPUを停止状態にして待機
    while(1) {
        __asm__("hlt");
    }
}
