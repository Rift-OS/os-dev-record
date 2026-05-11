//カーネル
void kernel_main(){
    unsigned short *vrom = (unsigned short *)0xb8000;

    //表示したい文字列
    const char *str = "Hello, world!";
    for (int i = 0; str[1] != '\0'; i++) {
        //文字コード
        video_memory[i] = (unsigned short)str[i] | (0x0a << 8);
    }
    //CPUを停止する
    while(1){
        __asm__("hlt");
    }
}