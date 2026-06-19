OUTPUT_FORMAT("binary")
ENTRY(start)

SECTIONS
{
    /* MBRが展開されるアドレス 0x7c00 基準 */
    . = 0x7c00;

    /* 先頭512バイトのブートセクタ */
    .boot : {
        *(.boot)
    }

    /* カーネルのプログラム本体 */
    .text : {
        *(.text)
    }

    /* C++の定数や読み取り専用データ（文字列リテラルなど） */
    .rodata : {
        *(.rodata*)
    }

    /* 初期化済みデータ */
    .data : {
        *(.data)
    }

    /* 未初期化変数（BSS領域） */
    .bss : {
        *(.bss)
        *(COMMON)
    }

    /* 不要なGCC/G++の例外枠セクションを破棄（エラー防止） */
    /DISCARD/ : {
        *(.eh_frame)
        *(.comment)
    }
}
