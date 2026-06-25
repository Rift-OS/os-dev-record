OUTPUT_FORMAT("elf32-i386")
ENTRY(kernel_entry)

SECTIONS
{
    . = 0x00100000;
    __kernel_start = .;

    .text.entry : ALIGN(0x1000) {
        KEEP(*(.text.entry))
    }
    __text_entry_end = .;

    .text : {
        *(.text*)
    }
    __text_end = .;

    .rodata : ALIGN(0x1000) {
        *(.rodata*)
    }
    __rodata_end = .;

    .data : ALIGN(0x1000) {
        *(.data*)
    }
    __data_end = .;

    .bss : ALIGN(0x1000) {
        __bss_start = .;
        *(COMMON)
        *(.bss*)
        __bss_end = .;
    }

    __kernel_end = .;

    /DISCARD/ : {
        *(.note.GNU-stack)
        *(.gnu_debuglink)
        *(.gnu_debugdata)
    }
}
