bits 32

global kernel_entry
global load_idt
global asm_mouse_handler

extern kernel_main
extern c_mouse_handler

section .text.entry
kernel_entry:
    cli
    ; スタックポインタの初期化
    mov esp, _sys_stack
    
    ; カーネルメインへ移行
    call kernel_main
    
.halt:
    hlt
    jmp .halt

section .text
; IDTを登録する関数
load_idt:
    mov edx, [esp + 4]
    lidt [edx]
    ret

; マウス割り込み（IRQ12）のエントリポイント
asm_mouse_handler:
    pushad
    push ds
    push es
    push fs
    push gs

    ; カーネルデータセグメントの割り当て (0x10)
    mov ax, 0x10
    mov ds, ax
    mov es, ax

    ; C++側のハンドラを呼び出し
    call c_mouse_handler

    pop gs
    pop fs
    pop es
    pop ds
    popad
    iretd

section .bss
align 16
resb 8192 ; 8KB stack
_sys_stack:
