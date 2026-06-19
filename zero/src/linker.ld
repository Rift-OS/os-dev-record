OUTPUT_FORMAT("binary")
ENTRY(start)

SECTIONS
{
    . = 0x7c00;

    /* 最初の512バイト（ブートセクタ） */
    .boot : {
        *(.boot)
    }

    /* 512バイト目以降に配置されるカーネルコード */
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
