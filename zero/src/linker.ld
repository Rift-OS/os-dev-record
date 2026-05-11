ENTRY(kernel_main) /* 最初に実行する関数名 */

SECTIONS
{
    /* 1MB（0x100000）の場所から配置するのがOSの定番 */
    . = 1M;

    .text : { *(.text) }   /* プログラム本体 */
    .data : { *(.data) }   /* データ */
    .bss  : { *(.bss)  }   /* 未初期化変数 */
}