org 0x7C00
bits 16

section .text

boot_start:
    jmp 0x0000:init_segments

init_segments:
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00

    ; 1. ディスクからカーネル本体（512バイト目以降）を 0x0000:0x8000 に読み込む
    mov ah, 0x02        ; BIOS Read Sectors
    mov al, 15          ; 読み込むセクタ数
    mov ch, 0           ; シリンダ 0
    mov cl, 2           ; セクタ 2 から開始
    mov dh, 0           ; ヘッド 0
    mov bx, 0x8000      ; ES:BX = 0x0000:0x8000
    int 0x13
    jc disk_error

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

    ; 5. 32bitコードセグメントへジャンプ (pm_entryのドットを削除)
    jmp 0x08:pm_entry

disk_error:
    mov si, msg_error
loop_err:
    lodsb
    or al, al
    jz halt_loop
    mov ah, 0x0E
    int 0x10
    jmp loop_err
halt_loop:
    hlt
    jmp halt_loop

msg_error: db "Disk Read Error!", 0

; ============================================================================
; GDT (Global Descriptor Table) 定義
; ============================================================================
align 4
gdt_start:
    dd 0, 0             ; ヌル記述子
gdt_code:
    dw 0xFFFF, 0x0000
    db 0x00, 0x9A, 0xCF, 0x00
gdt_data:
    dw 0xFFFF, 0x0000
    db 0x00, 0x92, 0xCF, 0x00
gdt_end:

gdt_pointer:
    dw gdt_end - gdt_start - 1
    dd gdt_start

; ============================================================================
; 32bit 保護モード・初期設定
; ============================================================================
bits 32
pm_entry:               ; ドットを削除
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; 6. 一時領域(0x8000)から1MB(0x100000)へカーネルを転送
    mov esi, 0x8000
    mov edi, 0x100000
    mov ecx, 1920
    rep movsd

    ; 7. リンカスクリプトで1MBに配置されている kernel_entry へジャンプ
    jmp 0x08:0x100000

; ブートシグネチャ（512バイトぴったりにする）
times 510-($-$$) db 0
dw 0xAA55
