# ビルド手順
shellscriptを使う
まずgcc,nasm,binutils,genisoimage
## shellscriptがある場合
- shell scriptがあっても手動でやる必要があります
```bash
chmod +x build.sh
./build.sh
```
## ない場合
```bash
# 1. build.sh というファイルを作成して中身を書き込む
cat << 'EOF' > build.sh
#!/bin/bash
nasm -f elf32 boot.asm -o boot.o
g++ -m32 -ffreestanding -fno-pic -fno-exceptions -fno-rtti -c console.cpp -o console.o
g++ -m32 -ffreestanding -fno-pic -fno-exceptions -fno-rtti -c keyboard.cpp -o keyboard.o
g++ -m32 -ffreestanding -fno-pic -fno-exceptions -fno-rtti -c kernel.cpp -o kernel.o
ld -m elf_i386 -T linker.ld boot.o console.o keyboard.o kernel.o -o OS.img
mkdir -p iso_root
cp OS.img iso_root/
genisoimage -R -b OS.img -no-emul-boot -boot-load-size 16 -o myos.iso iso_root
qemu-system-i386 -cdrom notepad.iso
EOF

# 2. 実行権限を付ける
chmod +x build.sh

# 3. 実行する
./build.sh
```
ない場合のために中身をつけています
## 私がビルドした環境
- **Ubuntu**です.
