; bootloader1.asm
[org 0x7C00]
[BITS 16]

; Display loading message
mov si, message
call print_string

; Load stage 2 bootloader from sector 1 (512 bytes) into 0x8000
mov ah, 0x02        ; BIOS read sector function
mov al, 1           ; Number of sectors to read
mov ch, 0           ; Cylinder
mov cl, 2           ; Sector number (BIOS is 1-based, so sector 2 = 2nd sector)
mov dh, 0           ; Head
mov dl, 0x00        ; Drive 0 (floppy)

mov ax, 0x0800      ; Correct way to set segment
mov es, ax
mov bx, 0x0000      ; Offset

int 0x13            ; BIOS disk interrupt
jc load_fail        ; If carry flag set, loading failed

jmp 0x0800:0000     ; Jump to loaded second stage

load_fail:
mov si, fail_msg
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

message db "Loading Stage 2...", 0
fail_msg db "Failed to load Stage 2!", 0

times 510 - ($ - $$) db 0
dw 0xAA55