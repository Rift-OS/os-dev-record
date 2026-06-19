[bits 16]
section .boot
global start
extern kernel_main

; カーネルが配置される直後のアドレス (0x7c00 + 512バイト = 0x7e00)
KERNEL_OFFSET equ 0x7e00

start:
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00

    mov [BOOT_DRIVE], dl

    ; 画面クリア
    mov ax, 0x0003
    int 10h

    ; ディスクの2セクタ目（カーネル部分）をメモリの 0x7e00 に読み込む
    mov bx, KERNEL_OFFSET
    mov dh, 10                  ; 多めに10セクタ分読み込む
    mov dl, [BOOT_DRIVE]
    call disk_load

    ; A20ラインの有効化
    in al, 0x92
    or al, 2
    out 0x92, al

    cli
    lgdt [gdt_descriptor]

    mov eax, cr0
    or eax, 1
    mov cr0, eax

    jmp CODE_SEG:init_pm

disk_load:
    push dx
    mov ah, 0x02
    mov al, dh
    mov ch, 0x00
    mov dh, 0x00
    mov cl, 0x02                ; 2セクタ目から読み込み
    int 0x13
    jc disk_error
    pop dx
    ret

disk_error:
    mov ax, 0x0e45              ; エラーなら 'E' を表示
    int 0x10
    jmp $

[bits 32]
init_pm:
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov ebp, 0x90000
    mov esp, ebp

    call kernel_main            ; 0x7e00 以降にあるカーネル本体を実行
    jmp $

BOOT_DRIVE: db 0

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

times 510-($-$$) db 0
dw 0xaa55
