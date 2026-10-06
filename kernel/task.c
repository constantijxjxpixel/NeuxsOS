#include "task.h"
#include "pmm.h"
#include "vga.h"
#include "timer.h"
#include "speaker.h"
#include <stdint.h>

struct task tasks[TASK_MAX];
int cur_task = 0;

extern void task_yield(void);
void trampoline(void);

void task_init(void){
    for (int i=0;i<TASK_MAX;i++){ tasks[i].used=0; tasks[i].state=0; tasks[i].name[0]=0; tasks[i].esp=0; }
}

int task_spawn(const char *name, void (*fn)(void)){
    int i=-1;
    for (int k=0;k<TASK_MAX;k++) if(!tasks[k].used){ i=k; break; }
    if (i<0) return -1;
    uint32_t page = pmm_alloc_page();
    if (!page) return -1;
    uint32_t *f = (uint32_t*)(page + 4096 - 68);
    f[0]=0x10; f[1]=0x10; f[2]=0x10; f[3]=0x10;
    f[4]=0; f[5]=0; f[6]=0; f[7]=0; f[8]=0; f[9]=0; f[10]=0; f[11]=0;
    f[12]=0; f[13]=0;
    f[14]=(uint32_t)trampoline;
    f[15]=0x08;
    f[16]=0x202;
    tasks[i].pid=(uint32_t)i;
    tasks[i].state=TASK_READY;
    tasks[i].esp=(uint32_t)f;
    tasks[i].stack=page;
    tasks[i].sleep_until=0;
    tasks[i].fn=fn;
    tasks[i].used=1;
    int k=0; while(name[k]&&k<11){ tasks[i].name[k]=name[k]; k++; } tasks[i].name[k]=0;
    return i;
}

void trampoline(void){
    tasks[cur_task].fn();
    task_exit();
}

void task_exit(void){
    tasks[cur_task].state = TASK_DEAD;
    task_yield();
    for(;;);
}

void task_sleep(uint32_t seconds){
    tasks[cur_task].state = TASK_SLEEPING;
    tasks[cur_task].sleep_until = timer_get_ticks() + seconds*100;
    task_yield();
}

void task_kill(int pid){
    if (pid<0 || pid>=TASK_MAX || !tasks[pid].used) return;
    tasks[pid].state = TASK_DEAD;
    if (pid == cur_task) task_yield();
}

void clock_task(void){
    const char *s = "|/-\\";
    int i = 0;
    for(;;){
        vga_putc_at(79,0,s[i&3],VGA_LGREEN,VGA_BLACK);
        i++;
        task_sleep(1);
    }
}

void beeper_task(void){
    for(;;){
        speaker_beep(1200,60);
        task_sleep(3);
    }
}

void counter_task(void){
    uint32_t c = 0;
    for(;;){
        uint32_t v = c % 10000;
        char d[5];
        d[0]=(char)('0'+v/1000); d[1]=(char)('0'+(v/100)%10);
        d[2]=(char)('0'+(v/10)%10); d[3]=(char)('0'+v%10); d[4]=0;
        for(int i=0;i<4;i++) vga_putc_at(75+i,0,d[i],VGA_LCYAN,VGA_BLACK);
        c++;
        task_sleep(1);
    }
}
