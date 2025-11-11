; message.asm - Boot sector that prints a message

[org 0x7c00]      ; BIOS loads boot sector to memory address 0x7C00
[BITS 16]         ; 16-bit real mode

mov si, msg       ; Load address of the message into SI
call print        ; Call print function

jmp $             ; Infinite loop after printing

print:
    lodsb         ; Load byte from DS:SI into AL and increment SI
    or al, al     ; Check if AL == 0 (null terminator)
    jz done
    mov ah, 0x0E  ; BIOS teletype output function
    int 0x10      ; Print AL to screen
    jmp print
done:
    ret

msg db "Hello, Chaitanya!", 0

; Pad to 512 bytes
times 510 - ($ - $$) db 0
dw 0xAA55