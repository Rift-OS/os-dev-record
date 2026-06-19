OUTPUT_FORMAT("binary")
ENTRY(start)

SECTIONS
{
    /* すべてのアドレスをブート開始位置の 0x7c00 基準で固定する */
    . = 0x7c00;

    .boot : {
        *(.boot)
    }

    .text : {
        *(.text)
    }
    .data : {
        *(.data)
    }
    .bss : {
        *(.bss)
    }
}
