# Lab 1C - Bootloader Loading a Kernel

## 🧠 Core Idea

We extended the bootloader to read multiple sectors from disk and jump to a loaded kernel.

## 📁 File: `os_boot.asm`

## 🔧 Commands

```bash
nasm -f bin os_boot.asm -o os_boot.bin
```

## 📝 Reasoning

* BIOS `int 0x13` is used to load kernel sectors from disk
* Loads into memory at `0x10000` (segment `0x1000`)
* Bootloader transfers control to kernel via `jmp 0x1000:0000`
* Demonstrates bootstrapping from real mode to a freestanding binary