#include <cstring>

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef int int32_t;

// ============================================================================
// 外部アセンブラ関数およびCハンドラの extern "C" 宣言
// ============================================================================
extern "C" {
    void load_idt(uint32_t idt_ptr);
    void asm_mouse_handler();
    void kernel_main();
    __attribute__((force_align_arg_pointer)) void c_mouse_handler();
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
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();

    outb(PIC1_DATA, 0x20);
    io_wait();
    outb(PIC2_DATA, 0x28);
    io_wait();

    outb(PIC1_DATA, 0x04);
    io_wait();
    outb(PIC2_DATA, 0x02);
    io_wait();

    outb(PIC1_DATA, 0x01);
    io_wait();
    outb(PIC2_DATA, 0x01);
    io_wait();

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
#define MOUSE_IRQ_VECTOR  0x2C

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
    if (type == 1) {
        while (timeout--) {
            if ((inb(MOUSE_PORT_CMD) & MOUSE_STATUS_IBF) == 0) return;
        }
    } else {
        while (timeout--) {
            if ((inb(MOUSE_PORT_CMD) & MOUSE_STATUS_OBF) != 0) return;
        }
    }
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
    g_mouse.x = 40; 
    g_mouse.y = 12;
    g_mouse.buttons = 0;

    // 1. マウスインターフェースを有効化 (0xA8)
    mouse_wait_signal(1);
    outb(MOUSE_PORT_CMD, 0xA8);

    // 2. コントローラのコンフィグレーション（コマンドバイト）を読み出す (0x20)
    mouse_wait_signal(1);
    outb(MOUSE_PORT_CMD, 0x20);
    mouse_wait_signal(0);
    uint8_t status = inb(MOUSE_PORT_DATA);

    // 3. IRQ12（マウス割り込み）の許可ビット(ビット1)を1にする
    status |= 0x02;  // マウス割り込み有効化
    status &= ~0x20; // マウス無効化ビットを解除

    // 4. 変更したコンフィグレーションを書き戻す (0x60)
    mouse_wait_signal(1);
    outb(MOUSE_PORT_CMD, 0x60);
    mouse_wait_signal(1);
    outb(MOUSE_PORT_DATA, status);

    // 5. マウスに「デフォルト設定に戻す」コマンドを送る (0xF6)
    mouse_write(0xF6);
    mouse_read(); // ACK (0xFA) の受け取り

    // 6. マウスに「データ送信を開始しろ」と命じる (0xF4)
    mouse_write(0xF4);
    mouse_read(); // ACK (0xFA) の受け取り
}

// ============================================================================
// 割り込みハンドラ (C++ 側実装)
// ============================================================================
extern "C" __attribute__((force_align_arg_pointer)) void c_mouse_handler() {
    uint8_t status = inb(MOUSE_PORT_CMD);
    
    if (!(status & MOUSE_STATUS_OBF) || !(status & 0x20)) {
        outb(PIC1_COMMAND, 0x20);
        outb(PIC2_COMMAND, 0x20);
        return;
    }

    uint8_t data = inb(MOUSE_PORT_DATA);

    if (g_mouse.phase == 0) {
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

        if (g_mouse.packet[0] & MOUSE_X_SIGN_BIT) move_x |= 0xFFFFFF00;
        if (g_mouse.packet[0] & MOUSE_Y_SIGN_BIT) move_y |= 0xFFFFFF00;

        g_mouse.x += move_x;
        g_mouse.y -= move_y; // Y軸方向の反転制御

        // VGAテキストモードの標準画面サイズ (80x25) にクランプ
        if (g_mouse.x < 0)  g_mouse.x = 0;
        if (g_mouse.x >= 80) g_mouse.x = 79;
        if (g_mouse.y < 0)  g_mouse.y = 0;
        if (g_mouse.y >= 25) g_mouse.y = 24;
    }

    outb(PIC2_COMMAND, 0x20);
    outb(PIC1_COMMAND, 0x20);
}

// ============================================================================
// デバッグ用画面表示補助関数
// ============================================================================
void print_string(int x, int y, const char* str, uint8_t color) {
    volatile uint8_t* vram = (volatile uint8_t*)0xB8000;
    int offset = (y * 80 + x) * 2;
    while (*str) {
        vram[offset] = *str++;
        vram[offset + 1] = color;
        offset += 2;
    }
}

void print_int(int x, int y, int32_t num, uint8_t color) {
    char buf[12];
    int i = 10;
    buf[11] = '\0';
    
    int is_negative = 0;
    if (num < 0) {
        is_negative = 1;
        num = -num;
    } else if (num == 0) {
        buf[--i] = '0';
    }

    while (num > 0 && i > 0) {
        buf[--i] = '0' + (num % 10);
        num /= 10;
    }

    if (is_negative && i > 0) {
        buf[--i] = '-';
    }

    print_string(x, y, &buf[i], color);
}

// ============================================================================
// カーネルエントリーポイント
// ============================================================================
extern "C" void kernel_main() {
    volatile uint8_t* vram = (volatile uint8_t*)0xB8000;
    for (int i = 0; i < 80 * 25 * 2; i += 2) {
        vram[i] = ' ';
        vram[i + 1] = 0x07; // 黒背景・白文字
    }

    print_string(0, 0, "Rift-OS Mouse mozi.", 0x0A); // 緑文字表示

    init_pic();
    init_idt();
    init_mouse();

    asm volatile("sti");

    int32_t last_x = -1;
    int32_t last_y = -1;

    while (1) {
        print_string(0, 2, "X:     ", 0x0F);
        print_int(3, 2, g_mouse.x, 0x0E); // 黄色でX座標表示

        print_string(12, 2, "Y:     ", 0x0F);
        print_int(15, 2, g_mouse.y, 0x0E); // 黄色でY座標表示

        if (last_x != -1 && last_y != -1) {
            int old_offset = (last_y * 80 + last_x) * 2;
            if (!(last_y == 2 && last_x < 25) && !(last_y == 0)) {
                vram[old_offset] = ' ';
                vram[old_offset + 1] = 0x07;
            }
        }

        int32_t current_x = g_mouse.x;
        int32_t current_y = g_mouse.y;
        int new_offset = (current_y * 80 + current_x) * 2;
        
        if (!(current_y == 2 && current_x < 25) && !(current_y == 0)) {
            vram[new_offset] = 'X';      // カーソル記号
            vram[new_offset + 1] = 0x0C; // 赤色で描画
        }

        last_x = current_x;
        last_y = current_y;

        asm volatile("hlt");
    }
}
