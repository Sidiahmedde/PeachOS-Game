#include "timer.h"
#include "io/io.h"
#include "idt/idt.h"
#include "kernel.h"
#include "memory/memory.h"

#define PIT_CHANNEL0 0x40
#define PIT_COMMAND  0x43
#define PIT_IRQ      0x20

static volatile uint64_t tick_count = 0;

uint64_t timer_ticks()
{
    return tick_count;
}

void timer_sleep(uint32_t ticks)
{
    uint64_t target = tick_count + ticks;
    while (tick_count < target)
    {
        // Busy wait; with interrupts enabled this will advance
    }
}

void timer_handle_interrupt()
{
    tick_count++;
    outb(0x20, 0x20);
}

void timer_init(uint32_t frequency_hz)
{
    // PIT input clock ~1.193182 MHz
    uint32_t divisor = 1193182 / frequency_hz;
    outb(PIT_COMMAND, 0x36); // channel 0, lo/hi byte, rate generator, binary
    outb(PIT_CHANNEL0, divisor & 0xFF);
    outb(PIT_CHANNEL0, (divisor >> 8) & 0xFF);
}
