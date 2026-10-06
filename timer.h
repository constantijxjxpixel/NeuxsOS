#ifndef KERNEL_TIMER_H
#define KERNEL_TIMER_H
#include <stdint.h>
void timer_install(void);
void timer_interrupt(void);
uint32_t timer_get_ticks(void);
uint32_t timer_get_seconds(void);
void timer_sleep(uint32_t seconds);
void timer_wait_ms(uint32_t ms);
#endif
