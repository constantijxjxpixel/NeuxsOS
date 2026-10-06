#ifndef KERNEL_SCHED_H
#define KERNEL_SCHED_H
#include <stdint.h>
void sched_install(void);
uint32_t scheduler_tick(uint32_t frame, int is_timer);
#endif
