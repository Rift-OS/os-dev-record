#!/bin/bash

# エラーが発生したら即座にスクリプトを終了
set -e

# 出力ファイル名の定義
BOOT_BIN="boot.bin"
KERNEL_ENTRY_O="kernel_entry.o"
KERNEL_O="kernel.o"
KERNEL_BIN="kernel.bin"
ISO_DIR="iso_root"
ISO_IMG="os.iso"

echo "=== OS ISO Build Sequence Started ==="

# 1. ブートローダのコンパイル (NASM)
echo "[1/4] Assembling boot.asm..."
nasm -f bin boot.asm -o "$BOOT_BIN"

# 2. カーネルエントリのコンパイル (NASM ELF32)
echo "[2/4] Assembling kernel_entry.asm..."
nasm -f elf32 kernel_entry.asm -o "$KERNEL_ENTRY_O"

# 3. カーネル本体のコンパイル (GCC 32bitモード)
echo "[3/4] Compiling kernel.cpp..."
g++ -m32 -ffreestanding -O2 -fno-exceptions -fno-rtti -nostdlib -c kernel.cpp -o "$KERNEL_O"

# 4. カーネルバイナリのリンク
echo "[4/4] Linking kernel binary with linker.ld..."
ld -m elf_i386 -T linker.ld "$KERNEL_ENTRY_O" "$KERNEL_O" -o "$KERNEL_BIN"

# 5. ISO作成用の一時ディレクトリ構築
echo "Preparing ISO root directory..."
rm -rf "$ISO_DIR"
mkdir -p "$ISO_DIR"

# El Torito規格の仕様に合わせるため、ブートセクタとカーネルを結合したイメージを配置
cat "$BOOT_BIN" "$KERNEL_BIN" > "$ISO_DIR/boot.img"
# フロッピーエミュレーション用にサイズを固定（1440KB）
truncate -s 1440k "$ISO_DIR/boot.img"

# 6. genisoimage による ISO イメージの生成
echo "Generating ISO image using genisoimage..."
genisoimage -R -b boot.img -no-emul-boot -boot-load-size 4 -o "$ISO_IMG" "$ISO_DIR"

# 後片付け
echo "Cleaning up temporary files..."
rm -f "$BOOT_BIN" "$KERNEL_ENTRY_O" "$KERNEL_O" "$KERNEL_BIN"
rm -rf "$ISO_DIR"

echo "=== Build Successful: $ISO_IMG is generated ==="
echo "To run the OS, execute: qemu-system-i386 -cdrom $ISO_IMG"
