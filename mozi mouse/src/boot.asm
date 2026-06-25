bits 32

; ============================================================================
; 定数定義
; ============================================================================
%define KERNEL_CS       0x08
%define KERNEL_DS       0x10
%define KERNEL_STACK_SIZE 8192

; ============================================================================
; グローバルシンボル
; ============================================================================
global kernel_entry
global load_idt
global asm_mouse_handler

extern kernel_main
extern c_mouse_handler

; ============================================================================
; .bss セクション（スタック）
; ============================================================================
section .bss
align 16
__stack_bottom:
    resb KERNEL_STACK_SIZE
__stack_top:

; ============================================================================
; .text.entry セクション（エントリーポイント）
; ============================================================================
section .text.entry

kernel_entry:
    ; 割り込み禁止
    cli
    
    ; 方向フラグクリア
    cld
    
    ; スタックポインタの初期化（スタック最上部）
    mov esp, __stack_top
    
    ; スタックキャナリ設定（オーバーフロー検出用）
    mov dword [__stack_top - 4], 0xDEADBEEF
    
    ; カーネルデータセグメントの設定
    mov ax, KERNEL_DS
    mov ss, ax
    
    ; 不定なセグメントレジスタをリセット
    xor eax, eax
    mov fs, ax
    mov gs, ax
    
    ; カーネルメインへ移行
    call kernel_main
    
    ; 戻ってこない（安全のため無限ループ）
.halt:
    cli
    hlt
    jmp .halt

; ============================================================================
; .text セクション（プログラムコード）
; ============================================================================
section .text

; ============================================================================
; void load_idt(uint32_t idt_ptr)
; 
; IDT (Interrupt Descriptor Table) をロードする
; 引数: [esp + 4] = IDTPtr 構造体のアドレス
; ============================================================================
load_idt:
    push ebp
    mov ebp, esp
    
    ; idt_ptr アドレスを edx に取得
    mov edx, [ebp + 8]
    
    ; LIDT 命令で IDT をロード
    lidt [edx]
    
    pop ebp
    ret

; ============================================================================
; void asm_mouse_handler(void)
;
; マウス割り込み（IRQ12）のアセンブラ側エントリーポイント
; CPU が自動的に以下をスタックに積む:
;   [esp + 0]: EIP
;   [esp + 4]: CS
;   [esp + 8]: EFLAGS
;
; iretd で復帰時に自動的に復元される
; ============================================================================
asm_mouse_handler:
    ; 汎用レジスタを保存
    pushad
    
    ; セグメントレジスタを保存
    push ds
    push es
    push fs
    push gs
    
    ; 方向フラグクリア（C++ 側の memset や std:: 関数使用時に必要）
    cld
    
    ; カーネルデータセグメントの設定
    mov ax, KERNEL_DS
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    ; スタックアライメント確保（System V ABI 互換性）
    ; 32-bit では強制されないが、SSE 命令使用時に重要
    ; call 直前のスタックポインタが 16 バイトアライメント - 4 であること
    sub esp, 4
    
    ; C++ 側のマウスハンドラを呼び出し
    call c_mouse_handler
    
    ; スタックアライメントを復元
    add esp, 4
    
    ; セグメントレジスタを復元
    pop gs
    pop fs
    pop es
    pop ds
    
    ; 汎用レジスタを復元
    popad
    
    ; 割り込みから復帰（EIP, CS, EFLAGS を自動復元）
    iretd
