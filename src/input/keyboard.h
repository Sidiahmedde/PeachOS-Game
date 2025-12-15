#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>

/**
 * Initializes the PS/2 keyboard (first port) and clears the input buffer.
 */
void keyboard_init();

/**
 * IRQ1 handler entry point. Reads the scancode, translates to ASCII and stores
 * it in a small ring buffer.
 */
void keyboard_handle_interrupt();

/**
 * Returns non-zero when a character is buffered.
 */
int keyboard_has_char();

/**
 * Pops a character from the buffer. Returns 0 when empty.
 */
char keyboard_pop_char();

#endif
