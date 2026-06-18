[bits 16]

section .boot
global start
extern kernel_main  ; kernel.c の関数を呼び出せるようにする

start:
    ; セグメントレジスタとスタックの初期化
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00

    ; kernel.c のメイン関数を実行
    call kernel_main

    ; カーネルから戻ってきたら無限ループ
    jmp $

; 512バイトのセクタ末尾にマジックナンバーを配置するためのパディング
; (リンカで結合するため、timesでの埋め方はビルド環境に合わせる必要があります)
times 510-($-$$) db 0
dw 0xaa55
