; code assembled and copied into an array in gpt.cpp 
; used for creation of the protective MBR
[org 0x7C00]

start:
    mov si, message

.print_loop:
    lodsb
    cmp al, 0
    je .hang
    mov ah, 0x0E
    int 0x10
    jmp .print_loop

.hang:
    cli
    hlt
    jmp .hang

message db "This is not a bootable disk", 0