OUTPUT_FORMAT("elf32-i386") [cite: 1]
ENTRY(kernel_entry) [cite: 1]

SECTIONS
{
    . = 0x00100000; [cite: 1]
    __kernel_start = .; [cite: 1]

    .text.entry : ALIGN(0x1000) { [cite: 1]
        KEEP(*(.text.entry)) [cite: 1]
    } [cite: 1]
    __text_entry_end = .;

    .text : { [cite: 1]
        *(.text*) [cite: 1]
    } [cite: 1]
    __text_end = .;

    .rodata : ALIGN(0x1000) { [cite: 1]
        *(.rodata*) [cite: 1]
    } [cite: 1]
    __rodata_end = .;

    .data : ALIGN(0x1000) { [cite: 1]
        *(.data*) [cite: 1]
    } [cite: 1]
    __data_end = .;

    .bss : ALIGN(0x1000) { [cite: 1]
        __bss_start = .;
        *(COMMON)
        *(.bss*)
        __bss_end = .;
    }

    __kernel_end = .; [cite: 1]

    /DISCARD/ : { [cite: 1]
        *(.note.GNU-stack) [cite: 1]
        *(.gnu_debuglink) [cite: 1]
        *(.gnu_debugdata) [cite: 1]
    } [cite: 1]
}
