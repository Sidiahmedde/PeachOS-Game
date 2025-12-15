#include "screen.h"
#include "kernel.h"
#include "string/string.h"

extern uint16_t* video_mem;
extern uint16_t terminal_row;
extern uint16_t terminal_col;

static inline uint16_t vga_entry(char c, char colour)
{
    return (uint16_t)c | ((uint16_t)colour << 8);
}

void screen_init()
{
    // kernel already sets video_mem; nothing else needed yet
}

void screen_clear()
{
    for (int y = 0; y < VGA_HEIGHT; y++)
    {
        for (int x = 0; x < VGA_WIDTH; x++)
        {
            video_mem[y * VGA_WIDTH + x] = vga_entry(' ', SCREEN_COL_BG);
        }
    }
    terminal_row = 0;
    terminal_col = 0;
}

void screen_putc(int x, int y, char c, char colour)
{
    if (x < 0 || x >= VGA_WIDTH || y < 0 || y >= VGA_HEIGHT)
        return;
    video_mem[y * VGA_WIDTH + x] = vga_entry(c, colour);
}

void screen_print(int x, int y, const char* s, char colour)
{
    int idx = 0;
    while (s[idx])
    {
        screen_putc(x + idx, y, s[idx], colour);
        idx++;
    }
}
