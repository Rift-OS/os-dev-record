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

    ; A20ラインの有効化 (高速化のため簡易版)
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

    ; 32bitコードセグメントへ長距離ジャンプ (パイプラインクリア)
    jmp CODE_SEG:init_pm

[bits 32]
init_pm:
    ; 32bit環境用のセグメントレジスタ初期化
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov ebp, 0x90000
    mov esp, ebp

    ; kernel.c のメイン関数を実行
    call kernel_main

    ; 無限ループ
    jmp $

; --- GDT (Global Descriptor Table) 設定 ---
gdt_start:
    ; 空記述子
    dd 0x0
    dd 0x0

gdt_code:
    ; コードセグメント記述子 (基点:0, 限界:4GB, 属性:高特権・実行可・読込可)
    dw 0xffff
    dw 0x0
    db 0x0
    db 10011010b
    db 11001111b
    db 0x0

gdt_data:
    ; データセグメント記述子 (基点:0, 限界:4GB, 属性:高特権・書込可・読込可)
    dw 0xffff
    dw 0x0
    db 0x0
    db 10010010b
    db 11001111b
    db 0x0
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start

; 512バイトのブートセクタ末尾にマジックナンバーを配置
times 510-($-$$) db 0
dw 0xaa55
