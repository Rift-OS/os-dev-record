void kernel_main() {
    // 画面（VGAバッファ）の物理アドレス
    unsigned short *video_memory = (unsigned short *)0xb8000;
    
    // 表示したい文字列
    const char *str = "Hello, World!";
    
    // i番目の文字が '\0' になるまで繰り返す
    for (int i = 0; str[i] != '\0'; i++) {
        // 文字を書き込む（0x0a は黄緑色）
        video_memory[i] = (unsigned short)str[i] | (0x0a << 8);
    }
    
    // CPUを停止状態にして待機
    while(1) {
        __asm__("hlt");
    }
}
