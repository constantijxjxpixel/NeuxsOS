#ifndef KERNEL_TASK_H
#define KERNEL_TASK_H
#include <stdint.h>
#define TASK_MAX 8
#define TASK_READY 1
#define TASK_RUNNING 2
#define TASK_SLEEPING 3
#define TASK_DEAD 4
struct task {
    uint32_t pid;
    uint32_t state;
    uint32_t esp;
    uint32_t stack;
    uint32_t sleep_until;
    void (*fn)(void);
    char name[12];
    int used;
};
extern struct task tasks[TASK_MAX];
extern int cur_task;
void task_init(void);
int  task_spawn(const char *name, void (*fn)(void));
void task_kill(int pid);
void task_sleep(uint32_t seconds);
void task_exit(void);
void clock_task(void);
void beeper_task(void);
void counter_task(void);
#endif
