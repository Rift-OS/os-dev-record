[bits 16]
section .boot
global start
extern kernel_main

start:
    ; セグメントレジスタとスタックの初期化
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00

    ; 画面クリア (BIOS割り込み INT 10h)
    mov ax, 0x0003
    int 10h

    ; ISO起動時はEl Torito規格で後続セクタが自動ロードされているため、
    ; 自力でのディスク読み込み処理は行わず次へ進む。

    ; A20ラインの有効化
    in al, 0x92
    or al, 2
    out 0x92, al

    ; GDTの読み込み
    cli
    lgdt [gdt_descriptor]

    ; CR0レジスタのPEビットをセットしてプロテクトモードへ移行
    mov eax, cr0
    or eax, 1
    mov cr0, eax

    ; 32bitコードセグメントへ長距離ジャンプ
    jmp CODE_SEG:init_pm

[bits 32]
init_pm:
    ; 32bit用のセグメントレジスタ初期化
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov ebp, 0x90000
    mov esp, ebp

    ; C++で記述された kernel_main を実行
    call kernel_main

    ; 無限ループ
    jmp $

; --- GDT 設定 ---
gdt_start:
    dd 0x0, 0x0
gdt_code:
    dw 0xffff, 0x0
    db 0x0, 10011010b, 11001111b, 0x0
gdt_data:
    dw 0xffff, 0x0
    db 0x0, 10010010b, 11001111b, 0x0
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start

; 512バイトの末尾にシグネチャを配置
times 510-($-$$) db 0
dw 0xaa55
