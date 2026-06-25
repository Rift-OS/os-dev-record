typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef int int32_t;

// ============================================================================
// 外部アセンブラ関数の宣言 [cite: 1]
// ============================================================================
extern "C" {
    void load_idt(uint32_t idt_ptr); [cite: 1]
    void asm_mouse_handler(); [cite: 1]
    void kernel_main(); [cite: 1]
    void c_mouse_handler(); [cite: 1]
}

// ============================================================================
// I/O ポート制御マクロ [cite: 1]
// ============================================================================
inline void outb(uint16_t port, uint8_t val) {
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port)); [cite: 1]
}

inline uint8_t inb(uint16_t port) {
    uint8_t ret; [cite: 1]
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port)); [cite: 1]
    return ret; [cite: 1]
}

inline void io_wait() {
    outb(0x80, 0); [cite: 1]
}

// ============================================================================
// PIC (Priority Interrupt Controller) 関連定義 [cite: 1]
// ============================================================================
#define PIC1_COMMAND 0x0020
#define PIC1_DATA    0x0021
#define PIC2_COMMAND 0x00A0
#define PIC2_DATA    0x00A1

#define ICW1_INIT    0x10
#define ICW1_ICW4    0x01

#define PIC_EOI      0x20

void init_pic() {
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4); [cite: 1]
    io_wait(); [cite: 1]
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4); [cite: 1]
    io_wait(); [cite: 1]
    
    outb(PIC1_DATA, 0x20); [cite: 1]
    io_wait(); [cite: 1]
    outb(PIC2_DATA, 0x28); [cite: 1]
    io_wait(); [cite: 1]
    
    outb(PIC1_DATA, 0x04); [cite: 1]
    io_wait(); [cite: 1]
    outb(PIC2_DATA, 0x02); [cite: 1]
    io_wait(); [cite: 1]
    
    outb(PIC1_DATA, 0x01); [cite: 1]
    io_wait(); [cite: 1]
    outb(PIC2_DATA, 0x01); [cite: 1]
    io_wait(); [cite: 1]
    
    outb(PIC1_DATA, 0xFB); [cite: 1]
    outb(PIC2_DATA, 0xEF); [cite: 1]
}

// ============================================================================
// IDT (Interrupt Descriptor Table) 関連定義 [cite: 1]
// ============================================================================
#define KERNEL_CS 0x08
#define KERNEL_DS 0x10

#define IDT_FLAG_PRESENT    0x80
#define IDT_FLAG_RING0      0x00
#define IDT_FLAG_INT_GATE   0x0E

#define MOUSE_IRQ_VECTOR    0x2C

struct IDTEntry {
    uint16_t offset_lower; [cite: 1]
    uint16_t selector; [cite: 1]
    uint8_t  reserved; [cite: 1]
    uint8_t  type_attr; [cite: 1]
    uint16_t offset_upper; [cite: 1]
} __attribute__((packed)); [cite: 1]

struct IDTPtr {
    uint16_t limit; [cite: 1]
    uint32_t base; [cite: 1]
} __attribute__((packed)); [cite: 1]

volatile IDTEntry idt[256]; [cite: 1]
volatile IDTPtr idtp; [cite: 1]

void set_idt_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt[num].offset_lower = base & 0xFFFF; [cite: 1]
    idt[num].offset_upper = (base >> 16) & 0xFFFF; [cite: 1]
    idt[num].selector     = sel; [cite: 1]
    idt[num].reserved     = 0; [cite: 1]
    idt[num].type_attr    = flags; [cite: 1]
}

void init_idt() {
    idtp.limit = (sizeof(IDTEntry) * 256) - 1; [cite: 1]
    idtp.base  = (uint32_t)&idt; [cite: 1]
    
    for (int i = 0; i < 256; i++) {
        set_idt_gate(i, 0, 0, 0); [cite: 1]
    }
    
    uint8_t flags = IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_INT_GATE; [cite: 1]
    set_idt_gate(MOUSE_IRQ_VECTOR, (uint32_t)asm_mouse_handler, KERNEL_CS, flags); [cite: 1]
    
    load_idt((uint32_t)&idtp); [cite: 1]
}

// ============================================================================
// PS/2 マウス関連定義 [cite: 1]
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
    MOUSE_OK = 0, [cite: 1]
    MOUSE_TIMEOUT_ERR = 1, [cite: 1]
    MOUSE_SYNC_ERR = 2 [cite: 1]
};

volatile MouseError g_mouse_error = MOUSE_OK; [cite: 1]

void mouse_wait_signal(uint8_t type) {
    uint32_t timeout = MOUSE_TIMEOUT; [cite: 1]
    
    if (type == 0) {
        while (timeout--) {
            if ((inb(MOUSE_PORT_STAT) & 0x01) == 0x01) { [cite: 1]
                return; [cite: 1]
            }
        }
    } else {
        while (timeout--) {
            if ((inb(MOUSE_PORT_STAT) & 0x02) == 0x00) { [cite: 1]
                return; [cite: 1]
            }
        }
    }
    g_mouse_error = MOUSE_TIMEOUT_ERR; [cite: 1]
}

void mouse_write(uint8_t data) {
    mouse_wait_signal(1); [cite: 1]
    outb(MOUSE_PORT_CMD, MOUSE_KBC_SEND); [cite: 1]
    mouse_wait_signal(1); [cite: 1]
    outb(MOUSE_PORT_DATA, data); [cite: 1]
}

uint8_t mouse_read() {
    mouse_wait_signal(0); [cite: 1]
    return inb(MOUSE_PORT_DATA); [cite: 1]
}

void init_mouse() {
    uint8_t status; [cite: 1]
    
    mouse_wait_signal(1); [cite: 1]
    outb(MOUSE_PORT_CMD, 0x20); [cite: 1]
    mouse_wait_signal(0); [cite: 1]
    status = inb(MOUSE_PORT_DATA); [cite: 1]
    
    status = (status | 0x02) & ~0x20; [cite: 1]
    
    mouse_wait_signal(1); [cite: 1]
    outb(MOUSE_PORT_CMD, 0x60); [cite: 1]
    mouse_wait_signal(1); [cite: 1]
    outb(MOUSE_PORT_DATA, status); [cite: 1]
    
    mouse_write(MOUSE_CMD_DEFAULT); [cite: 1]
    mouse_read(); [cite: 1]
    
    mouse_write(MOUSE_CMD_ENABLE); [cite: 1]
    mouse_read(); [cite: 1]
}

// ============================================================================
// マウスデータ解析 [cite: 1]
// ============================================================================
struct MouseState {
    volatile uint8_t cycle; [cite: 1]
    volatile uint8_t packet[3]; [cite: 1]
    volatile int32_t x; [cite: 1]
    volatile int32_t y; [cite: 1]
    volatile uint8_t left_button;   // bool から uint8_t に変更して安全性を向上
    volatile uint8_t right_button;
    volatile uint8_t middle_button;
} volatile g_mouse = {0, {0, 0, 0}, 320, 240, 0, 0, 0}; [cite: 1]

extern "C"
void c_mouse_handler() {
    uint8_t status = inb(MOUSE_PORT_STAT); [cite: 1]
    
    if ((status & 0x01) && (status & MOUSE_AUX_PORT_BIT)) { [cite: 1]
        uint8_t data = inb(MOUSE_PORT_DATA); [cite: 1]
        
        switch (g_mouse.cycle) { [cite: 1]
            case 0: [cite: 1]
                if ((data & MOUSE_SYNC_BIT) == MOUSE_SYNC_BIT) { [cite: 1]
                    g_mouse.packet[0] = data; [cite: 1]
                    g_mouse.cycle++; [cite: 1]
                } else { [cite: 1]
                    g_mouse_error = MOUSE_SYNC_ERR; [cite: 1]
                }
                break; [cite: 1]
                
            case 1: [cite: 1]
                g_mouse.packet[1] = data; [cite: 1]
                g_mouse.cycle++; [cite: 1]
                break; [cite: 1]
                
            case 2: [cite: 1]
                g_mouse.packet[2] = data; [cite: 1]
                g_mouse.cycle = 0; [cite: 1]
                
                g_mouse.left_button   = (g_mouse.packet[0] & MOUSE_LEFT_BTN) ? 1 : 0; [cite: 1]
                g_mouse.right_button  = (g_mouse.packet[0] & MOUSE_RIGHT_BTN) ? 1 : 0; [cite: 1]
                g_mouse.middle_button = (g_mouse.packet[0] & MOUSE_MIDDLE_BTN) ? 1 : 0; [cite: 1]
                
                int32_t move_x = (int32_t)g_mouse.packet[1]; [cite: 1]
                int32_t move_y = (int32_t)g_mouse.packet[2]; [cite: 1]
                
                if (g_mouse.packet[0] & MOUSE_X_SIGN_BIT) { [cite: 1]
                    move_x |= 0xFFFFFF00; [cite: 1]
                } [cite: 1]
                if (g_mouse.packet[0] & MOUSE_Y_SIGN_BIT) { [cite: 1]
                    move_y |= 0xFFFFFF00; [cite: 1]
                } [cite: 1]
                
                g_mouse.x += move_x; [cite: 1]
                g_mouse.y -= move_y; [cite: 1]
                
                if (g_mouse.x < 0) g_mouse.x = 0; [cite: 1]
                if (g_mouse.x >= 800) g_mouse.x = 799; [cite: 1]
                if (g_mouse.y < 0) g_mouse.y = 0; [cite: 1]
                if (g_mouse.y >= 600) g_mouse.y = 599; [cite: 1]
                break; [cite: 1]
        }
    }
    
    outb(PIC2_COMMAND, PIC_EOI); [cite: 1]
    outb(PIC1_COMMAND, PIC_EOI); [cite: 1]
}

// ============================================================================
// カーネルメインエントリー [cite: 1]
// ============================================================================
extern "C"
void kernel_main() {
    init_pic(); [cite: 1]
    init_idt(); [cite: 1]
    init_mouse(); [cite: 1]
    
    asm volatile("sti"); [cite: 1]
    
    while (1) {
        asm volatile("hlt"); [cite: 1]
    }
}
