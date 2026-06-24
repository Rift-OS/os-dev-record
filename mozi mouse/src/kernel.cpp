typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

// 外部アセンブラ関数の宣言
extern "C" {
    void load_idt(uint32_t idt_ptr);
    void asm_mouse_handler();
    void kernel_main();
    void c_mouse_handler();
}

// I/Oポート制御マクロ
inline void outb(uint16_t port, uint8_t val) {
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

inline void io_wait() {
    outb(0x80, 0);
}

// --- PIC (Priority Interrupt Controller) 関連定義 ---
#define PIC1_COMMAND 0x0020
#define PIC1_DATA    0x0021
#define PIC2_COMMAND 0x00A0
#define PIC2_DATA    0x00A1

#define ICW1_INIT    0x10
#define ICW1_ICW4    0x01

void init_pic() {
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4); io_wait();
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4); io_wait();
    
    // 割り込みベクタのオフセット設定 (マスター: 0x20, スレーブ: 0x28)
    outb(PIC1_DATA, 0x20); io_wait();
    outb(PIC2_DATA, 0x28); io_wait();
    
    // カスケード接続設定
    outb(PIC1_DATA, 4); io_wait();
    outb(PIC2_DATA, 2); io_wait();
    
    // 8086モード
    outb(PIC1_DATA, 0x01); io_wait();
    outb(PIC2_DATA, 0x01); io_wait();
    
    // IRQ12(マウス)とIRQ2(スレーブPIC接続用)以外のマスク（必要に応じて調整）
    outb(PIC1_DATA, 0xFB); // 11111011b (IRQ2許可)
    outb(PIC2_DATA, 0xEF); // 11101111b (IRQ12許可)
}

// --- IDT (Interrupt Descriptor Table) 関連定義 ---
struct IDTEntry {
    uint16_t offset_lower;
    uint16_t selector;
    uint8_t  zero;
    uint8_t  type_attr;
    uint16_t offset_upper;
} __attribute__((packed));

struct IDTPtr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

IDTEntry idt[256];
IDTPtr idtp;

void set_idt_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt[num].offset_lower = base & 0xFFFF;
    idt[num].offset_upper = (base >> 16) & 0xFFFF;
    idt[num].selector     = sel;
    idt[num].zero         = 0;
    idt[num].type_attr    = flags;
}

void init_idt() {
    idtp.limit = (sizeof(IDTEntry) * 256) - 1;
    idtp.base  = (uint32_t)&idt;
    
    // 全体をゼロクリア
    for(int i = 0; i < 256; i++) {
        set_idt_gate(i, 0, 0, 0);
    }
    
    // マウス割り込みは IRQ12 -> ベクタ 0x2C (0x28 + 4)
    // 0x08 はカーネルコードセグメント指定、0x8E は 32bit割り込みゲート(存在、DPL=0)
    set_idt_gate(0x2C, (uint32_t)asm_mouse_handler, 0x08, 0x8E);
    
    load_idt((uint32_t)&idtp);
}

// --- PS/2 マウス 関連定義 ---
#define MOUSE_PORT_DATA 0x60
#define MOUSE_PORT_STAT 0x64
#define MOUSE_PORT_CMD  0x64

void mouse_wait_signal(uint8_t type) {
    uint32_t timeout = 100000;
    if (type == 0) {
        // 読み込み可能まで待機
        while (timeout--) {
            if ((inb(MOUSE_PORT_STAT) & 1) == 1) return;
        }
    } else {
        // 書き込み可能まで待機
        while (timeout--) {
            if ((inb(MOUSE_PORT_STAT) & 2) == 0) return;
        }
    }
}

void mouse_write(uint8_t data) {
    mouse_wait_signal(1);
    outb(MOUSE_PORT_CMD, 0xD4); // マウスへデータ送信コマンド
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
    status = (inb(MOUSE_PORT_DATA) | 2) & ~0x20; // マウス割り込み有効化(bit1), マウス無効化解除(bit5)

    // コマンドバイトの書き戻し
    mouse_wait_signal(1);
    outb(MOUSE_PORT_CMD, 0x60);
    mouse_wait_signal(1);
    outb(MOUSE_PORT_DATA, status);

    // デフォルト設定の有効化
    mouse_write(0xF6);
    mouse_read; // ACKの受信

    // マウスデータ送信の開始
    mouse_write(0xF4);
    mouse_read; // ACKの受信
}

// --- マウスデータ解析 ---
struct MouseState {
    uint8_t cycle;
    uint8_t packet[3];
    int32_t x;
    int32_t y;
    bool left_button;
    bool right_button;
    bool middle_button;
} g_mouse;

// C++側の割り込み実体
void c_mouse_handler() {
    uint8_t status = inb(MOUSE_PORT_STAT);
    // 出力バッファが満杯かつ、それがマウスからのデータ(bit5=1)であるか確認
    if ((status & 1) && (status & 0x20)) {
        uint8_t data = inb(MOUSE_PORT_DATA);

        switch (g_mouse.cycle) {
            case 0:
                // 1バイト目は常にbit3が1であるべき（同期ズレ検出用）
                if ((data & 0x08) == 0x08) {
                    g_mouse.packet[0] = data;
                    g_mouse.cycle++;
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
                g_mouse.left_button   = (g_mouse.packet[0] & 0x01) != 0;
                g_mouse.right_button  = (g_mouse.packet[0] & 0x02) != 0;
                g_mouse.middle_button = (g_mouse.packet[0] & 0x04) != 0;

                // X, Y移動量の算出（符号拡張の処理を含む）
                int32_t move_x = g_mouse.packet[1];
                int32_t move_y = g_mouse.packet[2];

                if (g_mouse.packet[0] & 0x10) move_x |= 0xFFFFFF00; // X符号ビット
                if (g_mouse.packet[0] & 0x20) move_y |= 0xFFFFFF00; // Y符号ビット

                // 符号反転（画面座標系への最適化用、OSの仕様に合わせて調整可能）
                g_mouse.x += move_x;
                g_mouse.y -= move_y; 
                
                /*
                   ここで画面更新やUI等へ解析後のデータを反映させる
                */
                break;
        }
    }

    // PICに対して割り込み終了通知(EOI)を送信
    outb(PIC2_COMMAND, 0x20); // スレーブPIC
    outb(PIC1_COMMAND, 0x20); // マスターPIC
}

// --- カーネルメインエントリー ---
void kernel_main() {
    g_mouse.cycle = 0;
    g_mouse.x = 320; // 画面中央初期値などの想定
    g_mouse.y = 240;

    init_pic();
    init_idt();
    init_mouse();

    // CPUの割り込み許可
    asm volatile ("sti");

    // メインループ
    while (1) {
        asm volatile ("hlt");
    }
}
