#include "kernel.h"
#include <stddef.h>
#include <stdint.h>
#include "idt/idt.h"
#include "memory/heap/kheap.h"
#include "memory/paging/paging.h"
#include "memory/memory.h"
#include "string/string.h"
#include "fs/file.h"
#include "disk/disk.h"
#include "fs/pparser.h"
#include "disk/streamer.h"
#include "task/tss.h"
#include "input/keyboard.h"
#include "timer/timer.h"
#include "display/screen.h"

#include "gdt/gdt.h"
#include "config.h"
uint16_t* video_mem = 0;
uint16_t terminal_row = 0;
uint16_t terminal_col = 0;

uint16_t terminal_make_char(char c, char colour)
{
    return (colour << 8) | c;
}

void terminal_putchar(int x, int y, char c, char colour)
{
    video_mem[(y * VGA_WIDTH) + x] = terminal_make_char(c, colour);
}

void terminal_writechar(char c, char colour)
{
    if (c == '\n')
    {
        terminal_row += 1;
        terminal_col = 0;
        return;
    }
    if (c == '\b')
    {
        if (terminal_col > 0)
        {
            terminal_col -= 1;
            terminal_putchar(terminal_col, terminal_row, ' ', colour);
        }
        return;
    }

    terminal_putchar(terminal_col, terminal_row, c, colour);
    terminal_col += 1;
    if (terminal_col >= VGA_WIDTH)
    {
        terminal_col = 0;
        terminal_row += 1;
    }
}
void terminal_initialize()
{
    video_mem = (uint16_t*)(0xB8000);
    terminal_row = 0;
    terminal_col = 0;
    for (int y = 0; y < VGA_HEIGHT; y++)
    {
        for (int x = 0; x < VGA_WIDTH; x++)
        {
            terminal_putchar(x, y, ' ', 0);
        }
    }   
}



void print(const char* str)
{
    size_t len = strlen(str);
    for (int i = 0; i < len; i++)
    {
        terminal_writechar(str[i], 15);
    }
}


static struct paging_4gb_chunk* kernel_chunk = 0;

void panic(const char* msg)
{
    print(msg);
    while(1) {}
}

static void keyboard_flush_buffer()
{
    while (keyboard_has_char())
    {
        keyboard_pop_char();
    }
}

static void read_line(char* out, int max)
{
    int idx = 0;
    while (1)
    {
        while (!keyboard_has_char()) {}
        char c = keyboard_pop_char();
        if (c == '\n' || c == '\r')
        {
            print("\n");
            break;
        }
        if ((c == '\b' || c == 127))
        {
            if (idx > 0)
            {
                idx--;
                print("\b");
            }
            continue;
        }
        if (idx < max-1)
        {
            char s[2] = {c, 0};
            print(s);
            out[idx++] = c;
        }
    }
    out[idx] = 0;
}

static void itoa(int value, char* buffer, int base)
{
    if (base < 2 || base > 16)
    {
        buffer[0] = 0;
        return;
    }

    char tmp[32];
    int pos = 0;
    unsigned int v = (value < 0) ? -value : value;
    do
    {
        int digit = v % base;
        tmp[pos++] = (digit < 10) ? ('0' + digit) : ('A' + (digit - 10));
        v /= base;
    } while (v && pos < (int)sizeof(tmp)-1);

    if (value < 0)
    {
        tmp[pos++] = '-';
    }

    int out = 0;
    while (pos > 0)
    {
        buffer[out++] = tmp[--pos];
    }
    buffer[out] = 0;
}

static int snake_prompt_total_players()
{
    char buffer[8];
    print("How many players? [1]: ");
    read_line(buffer, sizeof(buffer));
    if (isdigit(buffer[0]))
    {
        int val = tonumericdigit(buffer[0]);
        if (val < 1) val = 1;
        if (val > 4) val = 4;
        return val;
    }
    return 1;
}

static int snake_run_single(const char* label)
{
    // Grid 40x18
    const int grid_w = 40;
    const int grid_h = 18;
    const int offset_x = 2;
    const int offset_y = 3;

    int snake_x[128];
    int snake_y[128];
    int snake_len = 5;
    int dir_x = 1, dir_y = 0;
    int alive = 1;
    for (int i = 0; i < snake_len; i++)
    {
        snake_x[i] = grid_w/2 - i;
        snake_y[i] = grid_h/2;
    }

    int food_x = 10;
    int food_y = 10;
    keyboard_flush_buffer();
    screen_clear();
    screen_print(offset_x, 0, "Snake kernel demo", SCREEN_COL_DEFAULT);
    screen_print(offset_x, 1, "Controls: WASD/arrows, Q ends your run", SCREEN_COL_DEFAULT);
    screen_print(offset_x, 2, label, SCREEN_COL_DEFAULT);

    uint64_t last_tick = timer_ticks();
    while (1)
    {
        // Input
        while (keyboard_has_char())
        {
            char c = keyboard_pop_char();
            if (c == 'w' && dir_y == 0) { dir_x = 0; dir_y = -1; }
            if (c == 's' && dir_y == 0) { dir_x = 0; dir_y = 1; }
            if (c == 'a' && dir_x == 0) { dir_x = -1; dir_y = 0; }
            if (c == 'd' && dir_x == 0) { dir_x = 1; dir_y = 0; }
            if (c == 'q' || c == 'Q') { return snake_len; }
        }

        // Tick pacing ~5 frames/sec
        if (timer_ticks() - last_tick < 12) { continue; }
        last_tick = timer_ticks();

        if (alive)
        {
            // Move snake tail -> head
            for (int i = snake_len-1; i > 0; i--)
            {
                snake_x[i] = snake_x[i-1];
                snake_y[i] = snake_y[i-1];
            }
            snake_x[0] += dir_x;
            snake_y[0] += dir_y;

            // Wrap
            if (snake_x[0] < 0) snake_x[0] = grid_w-1;
            if (snake_x[0] >= grid_w) snake_x[0] = 0;
            if (snake_y[0] < 0) snake_y[0] = grid_h-1;
            if (snake_y[0] >= grid_h) snake_y[0] = 0;

            // Self collision
            for (int i = 1; i < snake_len; i++)
            {
                if (snake_x[i] == snake_x[0] && snake_y[i] == snake_y[0])
                {
                    alive = 0;
                }
            }

            if (snake_x[0] == food_x && snake_y[0] == food_y)
            {
                if (snake_len < 120) snake_len++;
                food_x = (food_x * 37 + 3) % grid_w;
                food_y = (food_y * 17 + 1) % grid_h;
            }
        }

        // Draw
        screen_clear();
        screen_print(offset_x, 0, "Snake kernel demo", SCREEN_COL_DEFAULT);
        screen_print(offset_x, 1, "Controls: WASD/arrows, Q ends your run", SCREEN_COL_DEFAULT);
        screen_print(offset_x, 2, label, SCREEN_COL_DEFAULT);
        for (int y = 0; y < grid_h; y++)
        {
            for (int x = 0; x < grid_w; x++)
            {
                screen_putc(offset_x + x, offset_y + y, '.', SCREEN_COL_DEFAULT);
            }
        }
        // Food
        screen_putc(offset_x + food_x, offset_y + food_y, 'O', SCREEN_COL_FOOD);
        // Snake
        for (int i = 0; i < snake_len; i++)
        {
            screen_putc(offset_x + snake_x[i], offset_y + snake_y[i], i==0 ? '@' : '#', SCREEN_COL_P1);
        }
        int info_y = offset_y + grid_h + 1;
        char len_buf[16];
        itoa(snake_len, len_buf, 10);
        screen_print(offset_x, info_y, "Length:", SCREEN_COL_DEFAULT);
        screen_print(offset_x + 8, info_y, len_buf, SCREEN_COL_P1);
        screen_print(offset_x, info_y + 1, alive ? "Stay alive, eat O" : "Game Over - press Q", alive ? SCREEN_COL_DEFAULT : SCREEN_COL_FOOD);
    }

    return snake_len;
}

static void snake_game()
{
    const char* player_labels[] = {"Player 1", "Player 2", "Player 3", "Player 4"};
    int total_players = snake_prompt_total_players();
    if (total_players < 1) total_players = 1;
    if (total_players > 4) total_players = 4;

    int scores[4];
    for (int p = 0; p < total_players; p++)
    {
        scores[p] = snake_run_single(player_labels[p]);
    }

    // Show results back in the text terminal.
    terminal_initialize();
    print("Snake results:\n");
    int best_len = 0;
    int winners[4];
    int winner_count = 0;
    for (int p = 0; p < total_players; p++)
    {
        char num_buf[8];
        char len_buf[16];
        itoa(p+1, num_buf, 10);
        itoa(scores[p], len_buf, 10);
        print("  Player ");
        print(num_buf);
        print(": ");
        print(len_buf);
        print("\n");

        if (scores[p] > best_len)
        {
            best_len = scores[p];
            winner_count = 0;
            winners[winner_count++] = p;
        }
        else if (scores[p] == best_len)
        {
            winners[winner_count++] = p;
        }
    }

    print("\n");
    if (winner_count == 1)
    {
        char num_buf[8];
        char len_buf[16];
        itoa(winners[0]+1, num_buf, 10);
        itoa(best_len, len_buf, 10);
        print("Winner: Player ");
        print(num_buf);
        print(" with length ");
        print(len_buf);
        print("\n");
    }
    else
    {
        char len_buf[16];
        itoa(best_len, len_buf, 10);
        print("Winners: ");
        for (int i = 0; i < winner_count; i++)
        {
            char num_buf[8];
            itoa(winners[i]+1, num_buf, 10);
            print("Player ");
            print(num_buf);
            if (i < winner_count-1)
            {
                print(", ");
            }
        }
        print(" (length ");
        print(len_buf);
        print(")\n");
    }
    print("\n");
}

static void shell_prompt()
{
    print("> ");
}

static void shell_show_help()
{
    print("Commands:\n");
    print("  snake  - start snake demo (1-4 players take turns)\n");
    print("  clear  - clear screen\n");
    print("  help   - show this help\n");
    print("  quit   - halt\n");
}
struct tss tss;
struct gdt gdt_real[PEACHOS_TOTAL_GDT_SEGMENTS];
struct gdt_structured gdt_structured[PEACHOS_TOTAL_GDT_SEGMENTS] = {
    {.base = 0x00, .limit = 0x00, .type = 0x00},                // NULL Segment
    {.base = 0x00, .limit = 0xffffffff, .type = 0x9a},           // Kernel code segment
    {.base = 0x00, .limit = 0xffffffff, .type = 0x92},            // Kernel data segment
    {.base = 0x00, .limit = 0xffffffff, .type = 0xf8},              // User code segment
    {.base = 0x00, .limit = 0xffffffff, .type = 0xf2},             // User data segment
    {.base = (uint32_t)&tss, .limit=sizeof(tss), .type = 0xE9}      // TSS Segment
};
void kernel_main()
{
    terminal_initialize();
    print("Gaming kernel booting...\n");

    memset(gdt_real, 0x00, sizeof(gdt_real));
    gdt_structured_to_gdt(gdt_real, gdt_structured, PEACHOS_TOTAL_GDT_SEGMENTS);

    // Load the gdt
    gdt_load(gdt_real, sizeof(gdt_real));
    
    // Initialize the heap
    kheap_init();

    // Initialize filesystems
    fs_init();

    // Search and initialize the disks
    disk_search_and_init();

    // Initialize the interrupt descriptor table
    idt_init();
    keyboard_init();
    timer_init(60);
    screen_init();
    
    // Setup the TSS
    memset(&tss, 0x00, sizeof(tss));
    tss.esp0 = 0x600000;
    tss.ss0 = KERNEL_DATA_SELECTOR;

    // Load the TSS
    tss_load(0x28);

    // Setup paging
    kernel_chunk = paging_new_4gb(PAGING_IS_WRITEABLE | PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL);

    // Switch to kernel paging chunk
    paging_switch(paging_4gb_chunk_get_directory(kernel_chunk));

    // Enable paging
    enable_paging();

    // Enable the system interrupts
    enable_interrupts();

    // Minimal CLI to launch snake
    shell_show_help();
    keyboard_flush_buffer();
    char input[64];
    while (1)
    {
        shell_prompt();
        read_line(input, sizeof(input));
        if (strncmp(input, "snake", 5) == 0)
        {
            snake_game();
            keyboard_flush_buffer();
        }
        else if (strncmp(input, "clear", 5) == 0)
        {
            terminal_initialize();
        }
        else if (strncmp(input, "help", 4) == 0)
        {
            shell_show_help();
        }
        else if (strncmp(input, "quit", 4) == 0)
        {
            print("Halting. Reset the VM to restart.\n");
            while(1) {}
        }
        else
        {
            print("Unknown command. Type 'help'.\n");
        }
    }
}
