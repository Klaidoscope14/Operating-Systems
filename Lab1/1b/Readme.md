# Lab 1B - Bootloader with Message

## 🧠 Core Idea

We wrote a bootloader that prints a message to the screen using BIOS interrupt 0x10.

## 📁 File: `bootloader2.asm`

It prints "Booting ChaitanyaOS..." using a loop and BIOS teletype function.

## 🔧 Commands

```bash
nasm -f bin bootloader2.asm -o bootloader2.bin
qemu-system-i386 -drive format=raw,file=bootloader2.bin
```

## 📝 Reasoning

* Uses `int 0x10` with `AH=0x0E` to print characters from a message
* Still fits in 512 bytes including magic number `0xAA55`
* Good demonstration of BIOS services and low-level I/O