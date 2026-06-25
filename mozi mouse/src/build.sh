#!/bin/bash
set -e

BOOT_BIN="boot.bin"
KERNEL_O="kernel.o"
KERNEL_BIN="kernel.bin"
ISO_DIR="iso_root"
ISO_IMG="os.iso"

echo "=== OS ISO Build Sequence Started ==="

# 1. ブートローダとアセンブラ側のコンパイル (NASM)
echo "[1/3] Assembling boot.asm..."
nasm -f elf32 boot.asm -o "boot_elf.o"

# 2. カーネル本体のコンパイル
echo "[2/3] Compiling kernel.cpp..."
g++ -m32 -ffreestanding -O2 -fno-exceptions -fno-rtti -nostdlib -c kernel.cpp -o "$KERNEL_O"

# 3. リンカスクリプトを用いて結合・アドレス解決
echo "[3/3] Linking everything together..."
ld -m elf_i386 -T linker.ld "boot_elf.o" "$KERNEL_O" --oformat binary -o "$BOOT_BIN"

# 4. ISO作成用のディレクトリ構築
echo "Preparing ISO root directory..."
rm -rf "$ISO_DIR"
mkdir -p "$ISO_DIR"

# バイナリをフロッピーサイズに固定 (1.44MB = 1474560 バイト)
cp "$BOOT_BIN" "$ISO_DIR/boot.img"
truncate -s 1440k "$ISO_DIR/boot.img"

# 5. フロッピーエミュレーションモードでISOをビルド
echo "Generating ISO image using genisoimage..."
genisoimage -R -b boot.img -o "$ISO_IMG" "$ISO_DIR"

# 後片付け
echo "Cleaning up temporary files..."
rm -f "$BOOT_BIN" "boot_elf.o" "$KERNEL_O"
rm -rf "$ISO_DIR"

echo "=== Build Successful: $ISO_IMG is generated ==="
echo "To run the OS, execute: qemu-system-i386 -fda $ISO_IMG (or -cdrom $ISO_IMG)"
