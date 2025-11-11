; bootloader.asm
[org 0x7c00]        ; BIOS loads boot sector at 0x7C00
[BITS 16]           ; Use 16-bit instructions

mov si, message     ; SI = address of message

print_char:
    lodsb           ; Load byte at [SI] into AL, increment SI
    or al, al       ; Check if AL == 0 (end of string)
    jz halt
    mov ah, 0x0E    ; BIOS teletype function
    int 0x10        ; Call BIOS interrupt
    jmp print_char  ; Loop

halt:
    jmp $           ; Infinite loop

message db "Hello from Bootloader!", 0

; Fill the rest of the sector with zeros
times 510 - ($ - $$) db 0
dw 0xAA55           ; Boot sector magic number