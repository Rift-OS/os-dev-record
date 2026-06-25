bits 32

%define KERNEL_CS       0x08
%define KERNEL_DS       0x10
%define KERNEL_STACK_SIZE 8192

global kernel_entry
global load_idt
global asm_mouse_handler

extern kernel_main
extern c_mouse_handler

section .text.entry

kernel_entry:
cli
cld
mov esp, __stack_top
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
mov ebx, esp
and esp, 0xFFFFFFF0
call c_mouse_handler
mov esp, ebx
pop gs
pop fs
pop es
pop ds
popad
iretd

section .bss
align 16
__stack_bottom:
resb KERNEL_STACK_SIZE
__stack_top:
