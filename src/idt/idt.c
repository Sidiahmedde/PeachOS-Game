#include "idt.h"
#include "config.h"
#include "kernel.h"
#include "memory/memory.h"
#include "io/io.h"
#include "input/keyboard.h"
#include "timer/timer.h"
struct idt_desc idt_descriptors[PEACHOS_TOTAL_INTERRUPTS];
struct idtr_desc idtr_descriptor;

extern void idt_load(struct idtr_desc* ptr);

void idt_zero()
{
    print("Divide by zero error\n");
}

void idt_set(int interrupt_no, void* address)
{
    struct idt_desc* desc = &idt_descriptors[interrupt_no];
    desc->offset_1 = (uint32_t) address & 0x0000ffff;
    desc->selector = KERNEL_CODE_SELECTOR;
    desc->zero = 0x00;
    desc->type_attr = 0xEE;
        desc->offset_2 = (uint32_t) address >> 16;
}

extern void int21h();
extern void int20h();
extern void no_interrupt();

// Handler that prints Keyboard pressed!
void int21h_handler()
{
    keyboard_handle_interrupt();
}

// PIT timer handler
void int20h_handler()
{
    timer_handle_interrupt();
}

void no_interrupt_handler()
{
	outb(0x20, 0x20);	
}

void idt_init()
{
	memset(idt_descriptors, 0, sizeof(idt_descriptors));
	idtr_descriptor.limit= sizeof(idt_descriptors)-1;
	idtr_descriptor.base= (uint32_t) idt_descriptors;
	
	for(int i = 0; i<PEACHOS_TOTAL_INTERRUPTS; i++)
	{
		idt_set(i, no_interrupt);
	}
	
	idt_set(0, idt_zero);
	idt_set(0x20, int20h);
	idt_set(0x21, int21h);
	
	//load the interrupt descriptor table
	idt_load(&idtr_descriptor);
}
