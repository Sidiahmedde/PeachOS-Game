#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

void timer_init(uint32_t frequency_hz);
uint64_t timer_ticks();
void timer_sleep(uint32_t ticks);
void timer_handle_interrupt();

#endif
