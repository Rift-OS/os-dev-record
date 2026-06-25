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

void init_pic() {
    // ICW1: 初期化命令の開始
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();

    // ICW2: 割り込みベクトル番号のマッピングを変更 (0x20-0x27, 0x28-0x2F)
    outb(PIC1_DATA, 0x20);
    io_wait();
    outb(PIC2_DATA, 0x28);
    io_wait();

    // ICW3: マスタ・スレーブの接続ピン設定
    outb(PIC1_DATA, 0x04); // IRQ2にスレーブが接続
    io_wait();
    outb(PIC2_DATA, 0x02); // マスタのIRQ2に接続
    io_wait();

    // ICW4: 8086モードに設定
    outb(PIC1_DATA, 0x01);
    io_wait();
    outb(PIC2_DATA, 0x01);
    io_wait();

    // OCW1: 割り込みマスク（IRQ12 マウスと、スレーブへのカスケードピン IRQ2 を許可）
    outb(PIC1_DATA, 0xFB); // 11111011 (IRQ2許可)
    outb(PIC2_DATA, 0xEF); // 11101111 (IRQ12許可)
}

// ============================================================================
// IDT (Interrupt Descriptor Table) 関連定義
// ============================================================================
#define IDT_FLAG_PRESENT  0x80
#define IDT_FLAG_RING0    0x00
#define IDT_FLAG_INT_GATE 0x0E
#define KERNEL_CS         0x08
#define MOUSE_IRQ_VECTOR  0x2C // IRQ12 -> 0x20 + 12 = 0x2C

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
    idt[num].selector     = sel;
    idt[num].reserved     = 0;
    idt[num].type_attr    = flags;
    idt[num].offset_upper = (base >> 16) & 0xFFFF;
}

void init_idt() {
    idtp.limit = (sizeof(IDTEntry) * 256) - 1;
    idtp.base  = (uint32_t)&idt;

    for (int i = 0; i < 256; i++) {
        set_idt_gate(i, 0, 0, 0);
    }

    uint8_t flags = IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_INT_GATE;
    set_idt_gate(MOUSE_IRQ_VECTOR, (uint32_t)asm_mouse_handler, KERNEL_CS, flags);

    load_idt((uint32_t)&idtp);
}

// ============================================================================
// PS/2 マウスドライバ
// ============================================================================
#define MOUSE_PORT_DATA 0x60
#define MOUSE_PORT_CMD  0x64
#define MOUSE_STATUS_IBF 0x02
#define MOUSE_STATUS_OBF 0x01
#define MOUSE_KBC_SEND  0xD4

enum MouseError {
    MOUSE_OK = 0,
    MOUSE_TIMEOUT_ERR = 1,
};

volatile MouseError g_mouse_error = MOUSE_OK;

struct MousePacket {
    uint8_t  phase;
    uint8_t  packet[3];
    int32_t  x;
    int32_t  y;
    uint8_t  buttons;
};

volatile MousePacket g_mouse;

#define MOUSE_X_SIGN_BIT   0x10
#define MOUSE_Y_SIGN_BIT   0x20
#define MOUSE_LEFT_BUTTON  0x01
#define MOUSE_RIGHT_BUTTON 0x02

void mouse_wait_signal(uint8_t type) {
    uint32_t timeout = 100000;
    if (type == 1) { // 制御用書き込み準備完了を待つ (Input Buffer Empty)
        while (timeout--) {
            if ((inb(MOUSE_PORT_CMD) & MOUSE_STATUS_IBF) == 0) return;
        }
    } else { // 読み込みデータ到着を待つ (Output Buffer Full)
        while (timeout--) {
            if ((inb(MOUSE_PORT_CMD) & MOUSE_STATUS_OBF) != 0) return;
        }
    }
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
    g_mouse.phase = 0;
    g_mouse.x = 400; // 画面中央初期化
    g_mouse.y = 300;
    g_mouse.buttons = 0;

    // マウスインターフェース有効化
    mouse_wait_signal(1);
    outb(MOUSE_PORT_CMD, 0xA8);

    // デフォルト設定
    mouse_write(0xF6);
    mouse_read(); // ACKの受け取り

    // データ転送有効化
    mouse_write(0xF4);
    mouse_read(); // ACKの受け取り
}

// ============================================================================
// 割り込みハンドラ (C++ 側実装)
// ============================================================================
extern "C" void c_mouse_handler() {
    uint8_t status = inb(MOUSE_PORT_CMD);
    
    // 出力バッファが空、またはキーボードデータの場合は無視
    if (!(status & MOUSE_STATUS_OBF) || !(status & 0x20)) {
        outb(PIC1_COMMAND, 0x20);
        outb(PIC2_COMMAND, 0x20);
        return;
    }

    uint8_t data = inb(MOUSE_PORT_DATA);

    if (g_mouse.phase == 0) {
        // 第一バイトは特定のビット（通常bit3が1）が立っているか検証
        if ((data & 0x08) == 0x08) {
            g_mouse.packet[0] = data;
            g_mouse.phase = 1;
        }
    } else if (g_mouse.phase == 1) {
        g_mouse.packet[1] = data;
        g_mouse.phase = 2;
    } else if (g_mouse.phase == 2) {
        g_mouse.packet[2] = data;
        g_mouse.phase = 0;

        g_mouse.buttons = g_mouse.packet[0] & (MOUSE_LEFT_BUTTON | MOUSE_RIGHT_BUTTON);

        int32_t move_x = (int32_t)g_mouse.packet[1];
        int32_t move_y = (int32_t)g_mouse.packet[2];

        // 符号拡張
        if (g_mouse.packet[0] & MOUSE_X_SIGN_BIT) {
            move_x |= 0xFFFFFF00;
        }
        if (g_mouse.packet[0] & MOUSE_Y_SIGN_BIT) {
            move_y |= 0xFFFFFF00;
        }

        g_mouse.x += move_x;
        g_mouse.y -= move_y; // Y軸反転対応

        // 画面外クランプ (800x600)
        if (g_mouse.x < 0) g_mouse.x = 0;
        if (g_mouse.x >= 800) g_mouse.x = 799;
        if (g_mouse.y < 0) g_mouse.y = 0;
        if (g_mouse.y >= 600) g_mouse.y = 599;
    }

    // PICに割り込み終了(EOI)を通知
    outb(PIC2_COMMAND, 0x20);
    outb(PIC1_COMMAND, 0x20);
}

// ============================================================================
// カーネルエントリーポイント
// ============================================================================
extern "C" void kernel_main() {
    init_pic();
    init_idt();
    init_mouse();

    // 割り込み許可命令
    asm volatile("sti");

    while (1) {
        asm volatile("hlt");
    }
}
