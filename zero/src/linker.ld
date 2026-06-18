ENTRY(start) /* エントリポイントを boot.asm の start に変更 */

SECTIONS
{
    /* MBRが読み込まれるアドレス 0x7C00 に設定 */
    . = 0x7c00;

    .boot : { *(.boot) }

    .text : { *(.text) } /* プログラム本体 */
    .data : { *(.data) } /* データ */
    .bss  : { *(.bss) }  /* 未初期化変数 */
}
