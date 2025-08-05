; os_boot.asm - Bootloader
[org 0x7C00]
[BITS 16]

start:
    mov si, msg
    call print

    ; Load 10 sectors from disk to 0x1000:0000 (physical address 0x10000)
    mov ah, 0x02        ; BIOS function: Read sectors
    mov al, 10          ; Number of sectors to read
    mov ch, 0           ; Cylinder = 0
    mov cl, 2           ; Sector = 2 (we skip sector 1 which is the bootloader)
    mov dh, 0           ; Head = 0
    mov dl, 0x00        ; Drive = 0x00 (floppy)
    mov bx, 0x0000      ; Offset into segment
    mov ax, 0x1000      ; Segment
    mov es, ax          ; ES:BX = 0x1000:0000 = 0x10000

    int 0x13            ; BIOS interrupt to read disk sectors

    jmp 0x1000:0000     ; Jump to kernel at 0x10000

; --- Print string pointed by SI ---
print:
    lodsb               ; Load byte at DS:SI into AL, increment SI
    or al, al           ; Check for null terminator
    jz done
    mov ah, 0x0E        ; BIOS teletype output function
    int 0x10            ; Print AL
    jmp print

done:
    ret

msg db "Booting ChaitanyaOS...", 0

; --- Pad boot sector to 510 bytes ---
times 510 - ($ - $$) db 0

; --- Boot signature (magic number) ---
dw 0xAA55