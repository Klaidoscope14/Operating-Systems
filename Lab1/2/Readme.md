# Lab 2 - Dummy Operating System Kernel

## 🧠 Core Idea

We built a minimal dummy operating system kernel in C, linked with a custom linker script and loaded via a bootloader.

## 📁 Files:

* `kernel.c` — defines `print()` and `kernel_main()`
* `linker.ld` — links code to be loaded at `0x10000`
* `os_boot.asm` — bootloader loads kernel into memory

## 🔧 Build Commands

```bash
# Clean
rm -f kernel.o kernel.elf kernel.bin os_boot.bin os_image.img

# Assemble bootloader
nasm -f bin os_boot.asm -o os_boot.bin

# Compile kernel
x86_64-elf-gcc -ffreestanding -m32 -c kernel.c -o kernel.o

# Link kernel
x86_64-elf-ld -T linker.ld -m elf_i386 -o kernel.elf kernel.o

# Strip kernel to flat binary
x86_64-elf-objcopy -O binary -j .text -j .data -j .bss kernel.elf kernel.bin

# Build OS image
dd if=/dev/zero of=os_image.img bs=512 count=2880
dd if=os_boot.bin of=os_image.img conv=notrunc
dd if=kernel.bin of=os_image.img bs=512 seek=1 conv=notrunc

# Run in QEMU
qemu-system-i386 -drive format=raw,file=os_image.img
```

## 📝 Reasoning

* `-ffreestanding -m32`: Tell GCC not to assume any standard library or environment
* `linker.ld` aligns segments starting at 0x10000
* BIOS loads bootloader, bootloader loads kernel, control jumps to `kernel_main()`