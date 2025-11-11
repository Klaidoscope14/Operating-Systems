# Lab 1A - Simple Bootloader

## 🧠 Core Idea

In this lab, we created a simple bootloader in x86 Assembly that loops infinitely once booted. This demonstrated how to write the minimum 512-byte boot sector code recognized by the BIOS.

## 📁 File: `bootloader1.asm`

A simple loop with the BIOS boot signature.

## 🔧 Commands

```bash
nasm -f bin bootloader1.asm -o bootloader1.bin
qemu-system-i386 -drive format=raw,file=bootloader1.bin
```

## 📝 Reasoning

* BIOS loads first 512 bytes (boot sector) to `0x7C00`
* Ends with `0xAA55` magic number to mark it bootable
* `jmp loop` causes infinite loop after boot