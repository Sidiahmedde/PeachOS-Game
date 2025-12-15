[BITS 32]

global _start
global kernel_registers
extern kernel_main

CODE_SEG equ 0x08
DATA_SEG equ 0x10

_start:
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov ebp, 0x00200000
    mov esp, ebp

    ;PIC remap
    mov al, 00010001b
    out 0x20, al ; Tell master PIC
    mov al, 0x20
    out 0x21, al
    mov al, 0x04
    out 0x21, al
    mov al, 00000001b
    out 0x21, al
    ; End remap of the master PIC

    call kernel_main


    ; <<< END ADDED

    jmp $

kernel_registers:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov gs, ax
    mov fs, ax
    ret

times 512-($ - $$) db 0
