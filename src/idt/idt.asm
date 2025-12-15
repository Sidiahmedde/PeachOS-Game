section .asm

global enable_interrupts
global disable_interrupts

extern int21h_handler
extern int20h_handler
extern no_interrupt_handler

global int21h
global int20h
global idt_load
global no_interrupt

enable_interrupts:
	sti
	ret

disable_interrupts:
	cli
	ret

idt_load:
    push ebp
    mov ebp, esp

    mov ebx, [ebp+8]
    lidt [ebx]
    pop ebp
    ret
    
int21h:
	cli
	pushad
	call int21h_handler
	popad
	sti
	iret

int20h:
	cli
	pushad
	call int20h_handler
	popad
	sti
	iret
	
no_interrupt:
	cli
	pushad
	call no_interrupt_handler
	popad
	sti
	iret
