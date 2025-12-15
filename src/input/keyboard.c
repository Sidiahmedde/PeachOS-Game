#include "keyboard.h"
#include "io/io.h"
#include "kernel.h"
#include "idt/idt.h"
#include "memory/memory.h"

#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64
#define KEYBOARD_IRQ 0x21

#define KEYBOARD_BUFFER_SIZE 128

static char key_buffer[KEYBOARD_BUFFER_SIZE];
static int buffer_head = 0;
static int buffer_tail = 0;
static int extended_prefix = 0;

// Simple US QWERTY scancode set 1 map for printable keys we need
static const char scancode_ascii_map[128] = {
    0,  27, '1','2','3','4','5','6','7','8','9','0','-','=', '\b', // 0x0e = backspace
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n', 0,   // 0x1d = ctrl
    'a','s','d','f','g','h','j','k','l',';','\'','`', 0,'\\',
    'z','x','c','v','b','n','m',',','.','/', 0, 0, 0, ' ', // 0x39 = space
};

static void buffer_push(char c)
{
    int next_head = (buffer_head + 1) % KEYBOARD_BUFFER_SIZE;
    if (next_head == buffer_tail)
    {
        // Buffer full, drop the char
        return;
    }
    key_buffer[buffer_head] = c;
    buffer_head = next_head;
}

int keyboard_has_char()
{
    return buffer_head != buffer_tail;
}

char keyboard_pop_char()
{
    if (!keyboard_has_char())
        return 0;

    char c = key_buffer[buffer_tail];
    buffer_tail = (buffer_tail + 1) % KEYBOARD_BUFFER_SIZE;
    return c;
}

static char scancode_to_ascii(uint8_t scancode, int extended)
{
    if (extended)
    {
        // Arrow keys (set 1 extended 0xE0 prefix)
        switch (scancode)
        {
            case 0x48: return 'w'; // Up
            case 0x50: return 's'; // Down
            case 0x4B: return 'a'; // Left
            case 0x4D: return 'd'; // Right
            default: return 0;
        }
    }

    if (scancode < sizeof(scancode_ascii_map))
    {
        return scancode_ascii_map[scancode];
    }
    return 0;
}

void keyboard_handle_interrupt()
{
    uint8_t status = insb(KEYBOARD_STATUS_PORT);
    if (status & 0x01)
    {
        uint8_t scancode = insb(KEYBOARD_DATA_PORT);
        if (scancode == 0xE0)
        {
            extended_prefix = 1;
        }
        else if ((scancode & 0x80) == 0)
        {
            char ascii = scancode_to_ascii(scancode, extended_prefix);
            if (ascii)
            {
                buffer_push(ascii);
            }
            extended_prefix = 0;
        }
        else
        {
            // Key release; reset extended if it was part of the sequence
            extended_prefix = 0;
        }
    }

    // Send EOI
    outb(0x20, 0x20);
}

static void pic_remap()
{
    // Remap PIC to 0x20-0x27 and 0x28-0x2F (standard)
    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    outb(0x21, 0x20);
    outb(0xA1, 0x28);
    outb(0x21, 0x04);
    outb(0xA1, 0x02);
    outb(0x21, 0x01);
    outb(0xA1, 0x01);
    // Unmask timer (IRQ0) and keyboard (IRQ1); mask the rest
    outb(0x21, 0xFC); // 11111100b
    outb(0xA1, 0xFF);
}

void keyboard_init()
{
    buffer_head = buffer_tail = 0;
    pic_remap();
}
