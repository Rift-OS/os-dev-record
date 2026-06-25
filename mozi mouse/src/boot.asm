org 0x7C00
bits 16

section .boot

boot_start:
    jmp 0x0000:.init_segments

.init_segments:
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00

    ; 1. カーネル（2セクタ目以降）を一時的に 0x0000:0x8000 に読み込む
    ; (15セクタ分読み込み。カーネルが大きくなったらここを増やす)
    mov ah, 0x02        ; BIOS Read Sectors
    mov al, 15          ; 読み込むセクタ数
    mov ch, 0           ; シリンダ 0
    mov cl, 2           ; セクタ 2 から開始
    mov dh, 0           ; ヘッド 0
    ; dl はBIOSが起動ドライブ番号（0x00や0x80）を保持しているのでそのまま使用
    mov bx, 0x8000      ; ES:BX = 0x0000:0x8000
    int 0x13
    jc .error

    ; 2. A20ラインの有効化 (Fast A20)
    in al, 0x92
    or al, 2
    out 0x92, al

    ; 3. GDTのロード
    cli
    lgdt [gdt_pointer]

    ; 4. CR0 の PE ビットをセットして保護モードへ
    mov eax, cr0
    or eax, 1
    mov cr0, eax

    ; 5. 32bitコードセグメントへジャンブ
    jmp 0x08:.pm_entry

.error:
    mov si, msg_error
.loop_err:
    lodsb
    or al, al
    jz .halt
    mov ah, 0x0E
    int 0x10
    jmp .loop_err
.halt:
    hlt
    jmp .halt

msg_error: db "Disk Read Error!", 0

; ============================================================================
; GDT (Global Descriptor Table) 定義
; ============================================================================
align 4
gdt_start:
    ; ヌル記述子
    dd 0, 0
gdt_code:
    ; ベース: 0x0, リミット: 0xFFFFF, 属性: 32bit, コード, 実行/読込, 特権0
    dw 0xFFFF, 0x0000
    db 0x00, 0x9A, 0xCF, 0x00
gdt_data:
    ; ベース: 0x0, リミット: 0xFFFFF, 属性: 32bit, データ, 読込/書込, 特権0
    dw 0xFFFF, 0x0000
    db 0x00, 0x92, 0xCF, 0x00
gdt_end:

gdt_pointer:
    dw gdt_end - gdt_start - 1
    dd gdt_start

; ============================================================================
; 32bit 保護モード・エントリー
; ============================================================================
bits 32
.pm_entry:
    ; セグメントレジスタの初期化
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; 6. 一時領域(0x8000)から1MB(0x100000)へカーネル本体をコピー
    ; (15セクタ = 7680バイト = 1920ダブルワード)
    mov esi, 0x8000
    mov edi, 0x100000
    mov ecx, 1920
    rep movsd

    ; 7. リンカスクリプトで配置した kernel_entry へジャンプ
    jmp kernel_entry

; ブートセクタ（512バイト）のシグネチャ
times 510-($-$$) db 0
dw 0xAA55

; ============================================================================
; ここからカーネルエントリ（リンカによって 0x100000 に配置される）
; ============================================================================
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
    
    ; スタックポインタの初期化 [cite: 1]
    mov esp, __stack_top [cite: 1]
    
    ; スタックキャナリ設定 [cite: 1]
    mov dword [__stack_top - 4], 0xDEADBEEF [cite: 1]
    
    ; カーネルデータセグメントの設定 [cite: 1]
    mov ax, KERNEL_DS [cite: 1]
    mov ss, ax [cite: 1]
    
    xor eax, eax [cite: 1]
    mov fs, ax [cite: 1]
    mov gs, ax [cite: 1]
    
    call kernel_main [cite: 1]
    
.halt:
    cli [cite: 1]
    hlt [cite: 1]
    jmp .halt [cite: 1]

section .text

load_idt:
    push ebp [cite: 1]
    mov ebp, esp [cite: 1]
    mov edx, [ebp + 8] [cite: 1]
    lidt [edx] [cite: 1]
    pop ebp [cite: 1]
    ret [cite: 1]

asm_mouse_handler:
    pushad [cite: 1]
    push ds [cite: 1]
    push es [cite: 1]
    push fs [cite: 1]
    push gs [cite: 1]
    
    cld [cite: 1]
    
    mov ax, KERNEL_DS [cite: 1]
    mov ds, ax [cite: 1]
    mov es, ax [cite: 1]
    mov fs, ax [cite: 1]
    mov gs, ax [cite: 1]
    
    sub esp, 4 [cite: 1]
    call c_mouse_handler [cite: 1]
    add esp, 4 [cite: 1]
    
    pop gs [cite: 1]
    pop fs [cite: 1]
    pop es [cite: 1]
    pop ds [cite: 1]
    popad [cite: 1]
    iretd [cite: 1]

section .bss
align 16
__stack_bottom:
    resb KERNEL_STACK_SIZE [cite: 1]
__stack_top:
