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

    ; BIOSが設定した起動ドライブ番号(DL)を退避
    mov [boot_drive], dl

    ; ------------------------------------------------------------------------
    ; 【修正】カーネル本体(64セクタ分)を 0x0000:0x8000 へ1セクタずつ安全に読み込む
    ; ------------------------------------------------------------------------
    mov bx, 0x8000      ; 読み込み先の開始メモリ番地 (ES:BX)
    mov cl, 2           ; セクタ2 から開始 (セクタ1はブートローダー自身)
    mov ch, 0           ; シリンダ 0
    mov dh, 0           ; ヘッド 0
    mov si, 64          ; 読み込む総セクタ数 (32KB分)

read_sector_loop:
    mov di, 3           ; 各セクタごとに最大3回リトライ
retry_loop:
    mov ah, 0x02        ; BIOS Read Sectors
    mov al, 1           ; 1セクタずつ確実に読む
    mov dl, [boot_drive]
    int 0x13
    jnc read_success    ; エラーがなければ次へ

    ; エラー時はディスクをリセットしてリトライ
    xor ah, ah
    mov dl, [boot_drive]
    int 0x13
    dec di
    jnz retry_loop
    jmp disk_error      ; 3回連続失敗でエラー表示へ

read_success:
    ; 次のセクタを読み込むために、メモリ番地を512バイト進める
    add bx, 512
    inc cl              ; セクタ番号を+1
    cmp cl, 19          ; 標準的な1.44MB FDは1トラック18セクタまで
    jl next_step
    
    ; 18セクタを超えたらヘッドを切り替えるかシリンダを進める
    mov cl, 1           ; セクタ番号を1に戻す
    inc dh              ; ヘッドを1に進める
    cmp dh, 2           ; ヘッドは0と1の2面
    jl next_step
    mov dh, 0           ; ヘッドを0に戻す
    inc ch              ; シリンダを次へ進める

next_step:
    dec si
    jnz read_sector_loop ; 指定セクタ数分ループ

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

    ; 5. 32bitコードセグメントへジャンプ
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
boot_drive: db 0

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
pm_entry:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; 6. 一時領域(0x8000)から1MB(0x100000)へカーネルを転送 (32KB分 = 8192個のdword)
    mov esi, 0x8000
    mov edi, 0x100000
    mov ecx, 8192
    rep movsd

    ; 7. 1MBに配置されている kernel_entry へジャンプ
    jmp 0x08:0x100000

; ブートシグネチャ
times 510-($-$$) db 0
dw 0xAA55
