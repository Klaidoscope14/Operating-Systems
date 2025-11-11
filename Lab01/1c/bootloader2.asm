; bootloader2.asm
[org 0x8000]
[BITS 16]

mov si, msg
call print_string
jmp $

print_string:
    lodsb
    or al, al
    jz done
    mov ah, 0x0E
    int 0x10
    jmp print_string
done:
    ret

msg db "Welcome to Stage 2!", 0