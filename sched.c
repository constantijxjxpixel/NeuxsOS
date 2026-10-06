#include "sched.h"
#include "task.h"
#include "idt.h"
#include "timer.h"
#include <stdint.h>
static inline void outb(uint16_t p, uint8_t v){ __asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p)); }

extern void irq0_handler(void);

static int in_sched = 0;

uint32_t scheduler_tick(uint32_t frame, int is_timer){
    outb(0x20, 0x20);
    if (in_sched) return frame;
    in_sched = 1;

    if (is_timer) timer_interrupt();
    uint32_t t = timer_get_ticks();

    for (int i=0;i<TASK_MAX;i++)
        if (tasks[i].used && tasks[i].state==TASK_SLEEPING && t >= tasks[i].sleep_until)
            tasks[i].state = TASK_READY;

    if (tasks[cur_task].used) {
        tasks[cur_task].esp = frame;
        if (tasks[cur_task].state == TASK_RUNNING) tasks[cur_task].state = TASK_READY;
    }

    int n = -1;
    for (int s=1;s<=TASK_MAX;s++){
        int i = (cur_task + s) % TASK_MAX;
        if (tasks[i].used && tasks[i].state == TASK_READY){ n=i; break; }
    }
    if (n < 0) {
        if (tasks[cur_task].used) tasks[cur_task].state = TASK_RUNNING;
        in_sched = 0;
        return frame;
    }
    cur_task = n;
    tasks[n].state = TASK_RUNNING;
    in_sched = 0;
    return tasks[n].esp;
}

void sched_install(void){
    task_init();
    tasks[0].used=1;
    tasks[0].pid=0;
    tasks[0].state=TASK_RUNNING;
    tasks[0].esp=0;
    tasks[0].name[0]='i'; tasks[0].name[1]='d'; tasks[0].name[2]='l'; tasks[0].name[3]='e'; tasks[0].name[4]=0;
    idt_set_gate(32, (uint32_t)irq0_handler, 0x08, 0x8E);
}
