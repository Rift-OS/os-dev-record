#include <cstring>

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef int int32_t;

// ============================================================================
// 外部アセンブラ関数の宣言
// ============================================================================
extern "C" {
    void load_idt(uint32_t idt_ptr);
    void asm_mouse_handler();
    void kernel_main();
    void c_mouse_handler();
}

// ============================================================================
// I/O ポート制御マクロ
// ============================================================================
inline void outb(uint16_t port, uint8_t val) {
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

inline void io_wait() {
    outb(0x80, 0);
}

// ============================================================================
// PIC (Priority Interrupt Controller) 関連定義
// ============================================================================
#define PIC1_COMMAND 0x0020
#define PIC1_DATA    0x0021
#define PIC2_COMMAND 0x00A0
#define PIC2_DATA    0x00A1

#define ICW1_INIT    0x10
#define ICW1_ICW4    0x01

#define PIC_EOI      0x20

void init_pic() {
    // PIC1 (マスター) 初期化
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    
    // PIC2 (スレーブ) 初期化
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    
    // 割り込みベクタのオフセット設定
    // マスター: 0x20 (IRQ0-7), スレーブ: 0x28 (IRQ8-15)
    outb(PIC1_DATA, 0x20);
    io_wait();
    outb(PIC2_DATA, 0x28);
    io_wait();
    
    // カスケード接続設定
    // PIC1 の IRQ2 に PIC2 を接続
    outb(PIC1_DATA, 0x04);
    io_wait();
    outb(PIC2_DATA, 0x02);
    io_wait();
    
    // 8086 モード
    outb(PIC1_DATA, 0x01);
    io_wait();
    outb(PIC2_DATA, 0x01);
    io_wait();
    
    // マスク設定
    // PIC1: 0xFB = 11111011b (IRQ2 許可、その他マスク)
    // PIC2: 0xEF = 11101111b (IRQ12 許可、その他マスク)
    outb(PIC1_DATA, 0xFB);
    outb(PIC2_DATA, 0xEF);
}

// ============================================================================
// IDT (Interrupt Descriptor Table) 関連定義
// ============================================================================
#define KERNEL_CS 0x08
#define KERNEL_DS 0x10

#define IDT_FLAG_PRESENT    0x80
#define IDT_FLAG_RING0      0x00
#define IDT_FLAG_INT_GATE   0x0E
#define IDT_FLAG_INT32      0x08

#define MOUSE_IRQ_VECTOR    0x2C

struct IDTEntry {
    uint16_t offset_lower;
    uint16_t selector;
    uint8_t  reserved;
    uint8_t  type_attr;
    uint16_t offset_upper;
} __attribute__((packed));

struct IDTPtr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

volatile IDTEntry idt[256];
volatile IDTPtr idtp;

void set_idt_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt[num].offset_lower = base & 0xFFFF;
    idt[num].offset_upper = (base >> 16) & 0xFFFF;
    idt[num].selector     = sel;
    idt[num].reserved     = 0;
    idt[num].type_attr    = flags;
}

void init_idt() {
    // IDT テーブル初期化
    idtp.limit = (sizeof(IDTEntry) * 256) - 1;
    idtp.base  = (uint32_t)&idt;
    
    // 全エントリをゼロクリア
    for (int i = 0; i < 256; i++) {
        set_idt_gate(i, 0, 0, 0);
    }
    
    // マウス割り込み (IRQ12 -> ベクタ 0x2C)
    uint8_t flags = IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_INT_GATE;
    set_idt_gate(MOUSE_IRQ_VECTOR, (uint32_t)asm_mouse_handler, KERNEL_CS, flags);
    
    // IDT をロード
    load_idt((uint32_t)&idtp);
}

// ============================================================================
// PS/2 マウス関連定義
// ============================================================================
#define MOUSE_PORT_DATA  0x60
#define MOUSE_PORT_STAT  0x64
#define MOUSE_PORT_CMD   0x64

#define MOUSE_SYNC_BIT   0x08
#define MOUSE_LEFT_BTN   0x01
#define MOUSE_RIGHT_BTN  0x02
#define MOUSE_MIDDLE_BTN 0x04
#define MOUSE_X_SIGN_BIT 0x10
#define MOUSE_Y_SIGN_BIT 0x20
#define MOUSE_AUX_PORT_BIT 0x20

#define MOUSE_CMD_ENABLE 0xF4
#define MOUSE_CMD_DEFAULT 0xF6
#define MOUSE_KBC_SEND   0xD4

#define MOUSE_TIMEOUT    100000

enum MouseError {
    MOUSE_OK = 0,
    MOUSE_TIMEOUT_ERR = 1,
    MOUSE_SYNC_ERR = 2
};

volatile MouseError g_mouse_error = MOUSE_OK;

void mouse_wait_signal(uint8_t type) {
    uint32_t timeout = MOUSE_TIMEOUT;
    
    if (type == 0) {
        // 読み込み可能まで待機 (ステータス bit0 = 1)
        while (timeout--) {
            if ((inb(MOUSE_PORT_STAT) & 0x01) == 0x01) {
                return;
            }
        }
    } else {
        // 書き込み可能まで待機 (ステータス bit1 = 0)
        while (timeout--) {
            if ((inb(MOUSE_PORT_STAT) & 0x02) == 0x00) {
                return;
            }
        }
    }
    
    // タイムアウト
    g_mouse_error = MOUSE_TIMEOUT_ERR;
}

void mouse_write(uint8_t data) {
    mouse_wait_signal(1);
    outb(MOUSE_PORT_CMD, MOUSE_KBC_SEND);
    mouse_wait_signal(1);
    outb(MOUSE_PORT_DATA, data);
}

uint8_t mouse_read() {
    mouse_wait_signal(0);
    return inb(MOUSE_PORT_DATA);
}

void init_mouse() {
    uint8_t status;
    
    // コマンドバイトの読み込み
    mouse_wait_signal(1);
    outb(MOUSE_PORT_CMD, 0x20);
    mouse_wait_signal(0);
    status = inb(MOUSE_PORT_DATA);
    
    // bit1 (マウス割り込み有効化) = 1
    // bit5 (マウス無効化) = 0
    status = (status | 0x02) & ~0x20;
    
    // コマンドバイトの書き戻し
    mouse_wait_signal(1);
    outb(MOUSE_PORT_CMD, 0x60);
    mouse_wait_signal(1);
    outb(MOUSE_PORT_DATA, status);
    
    // デフォルト設定の有効化
    mouse_write(MOUSE_CMD_DEFAULT);
    mouse_read();  // ACK の受信
    
    // マウスデータ送信の開始
    mouse_write(MOUSE_CMD_ENABLE);
    mouse_read();  // ACK の受信
}

// ============================================================================
// マウスデータ解析
// ============================================================================
struct MouseState {
    volatile uint8_t cycle;
    volatile uint8_t packet[3];
    volatile int32_t x;
    volatile int32_t y;
    volatile bool left_button;
    volatile bool right_button;
    volatile bool middle_button;
} volatile g_mouse = {0, {0, 0, 0}, 320, 240, false, false, false};

extern "C"
void c_mouse_handler() {
    uint8_t status = inb(MOUSE_PORT_STAT);
    
    // 出力バッファが満杯かつ、マウスからのデータ (bit5=1) か確認
    if ((status & 0x01) && (status & MOUSE_AUX_PORT_BIT)) {
        uint8_t data = inb(MOUSE_PORT_DATA);
        
        switch (g_mouse.cycle) {
            case 0:
                // 1バイト目：同期ビット (bit3) が 1 であるべき
                if ((data & MOUSE_SYNC_BIT) == MOUSE_SYNC_BIT) {
                    g_mouse.packet[0] = data;
                    g_mouse.cycle++;
                } else {
                    // 同期ズレ：cycle を 0 に保持してスキップ
                    g_mouse_error = MOUSE_SYNC_ERR;
                }
                break;
                
            case 1:
                g_mouse.packet[1] = data;
                g_mouse.cycle++;
                break;
                
            case 2:
                g_mouse.packet[2] = data;
                g_mouse.cycle = 0;
                
                // ボタン状態の解析
                g_mouse.left_button   = (g_mouse.packet[0] & MOUSE_LEFT_BTN) != 0;
                g_mouse.right_button  = (g_mouse.packet[0] & MOUSE_RIGHT_BTN) != 0;
                g_mouse.middle_button = (g_mouse.packet[0] & MOUSE_MIDDLE_BTN) != 0;
                
                // X, Y 移動量の算出（符号拡張）
                int32_t move_x = (int32_t)g_mouse.packet[1];
                int32_t move_y = (int32_t)g_mouse.packet[2];
                
                // X 軸の符号拡張（bit 10 が符号ビット）
                if (g_mouse.packet[0] & MOUSE_X_SIGN_BIT) {
                    move_x |= 0xFFFFFF00;
                }
                
                // Y 軸の符号拡張（bit 11 が符号ビット）
                if (g_mouse.packet[0] & MOUSE_Y_SIGN_BIT) {
                    move_y |= 0xFFFFFF00;
                }
                
                // マウス座標の更新（画面座標系への変換）
                g_mouse.x += move_x;
                g_mouse.y -= move_y;  // Y 軸反転
                
                // 画面範囲のクランプ（800x600 を想定）
                if (g_mouse.x < 0) g_mouse.x = 0;
                if (g_mouse.x >= 800) g_mouse.x = 799;
                if (g_mouse.y < 0) g_mouse.y = 0;
                if (g_mouse.y >= 600) g_mouse.y = 599;
                
                break;
        }
    }
    
    // PIC に対して割り込み終了通知 (EOI) を送信
    outb(PIC2_COMMAND, PIC_EOI);  // スレーブ PIC
    outb(PIC1_COMMAND, PIC_EOI);  // マスター PIC
}

// ============================================================================
// カーネルメインエントリー
// ============================================================================
extern "C"
void kernel_main() {
    // マウス初期化
    init_pic();
    init_idt();
    init_mouse();
    
    // CPU の割り込み許可
    asm volatile("sti");
    
    // メインループ
    while (1) {
        asm volatile("hlt");
    }
}
