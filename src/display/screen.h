#ifndef SCREEN_H
#define SCREEN_H

#include <stdint.h>

void screen_init();
void screen_clear();
void screen_putc(int x, int y, char c, char colour);
void screen_print(int x, int y, const char* s, char colour);

// Handy colours (VGA low 4-bit foreground)
#define SCREEN_COL_DEFAULT 0x0F
#define SCREEN_COL_SNAKE   0x0A
#define SCREEN_COL_FOOD    0x0C
#define SCREEN_COL_BG      0x00
#define SCREEN_COL_P1      SCREEN_COL_SNAKE
#define SCREEN_COL_P2      0x0E

#endif
