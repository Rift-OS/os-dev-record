bits 32

%define KERNEL_CS       0x08
%define KERNEL_DS       0x10
%define KERNEL_STACK_SIZE 8192

global kernel_entry
global load_idt
global asm_mouse_handler

extern kernel_main
extern c_mouse_handler

; ============================================================================
; .text.entry セクション（1MBの先頭に配置される）
; ============================================================================
section .text.entry

kernel_entry:
    cli
    cld
    
    ; スタックポインタ初期化
    mov esp, __stack_top
    
    ; スタックキャナリ設定
    mov dword [__stack_top - 4], 0xDEADBEEF
    
    mov ax, KERNEL_DS
    mov ss, ax
    
    xor eax, eax
    mov fs, ax
    mov gs, ax
    
    call kernel_main
    
.halt:
    cli
    hlt
    jmp .halt

; ============================================================================
; .text セクション
; ============================================================================
section .text

load_idt:
    push ebp
    mov ebp, esp
    mov edx, [ebp + 8]
    lidt [edx]
    pop ebp
    ret


asm_mouse_handler:
    pushad
    push ds
    push es
    push fs
    push gs
    
    cld
    
    mov ax, KERNEL_DS
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    call c_mouse_handler   ; 余計な sub esp, 4 などを削除
    
    pop gs
    pop fs
    pop es
    pop ds
    popad
    iretd

; ============================================================================
; .bss セクション
; ============================================================================
section .bss
align 16
__stack_bottom:
    resb KERNEL_STACK_SIZE
__stack_top:
