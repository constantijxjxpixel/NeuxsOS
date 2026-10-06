#include "speaker.h"
#include "timer.h"
#include <stdint.h>
static inline void outb(uint16_t p, uint8_t v){ __asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p)); }
static inline uint8_t inb(uint16_t p){ uint8_t r; __asm__ volatile("inb %1,%0":"=a"(r):"Nd"(p)); return r; }
void speaker_beep(uint32_t freq, uint32_t ms){
    if (!freq) freq = 440;
    uint32_t d = 1193182 / freq;
    outb(0x43, 0xB6);
    outb(0x42, (uint8_t)(d & 0xFF));
    outb(0x42, (uint8_t)(d >> 8));
    uint8_t p = inb(0x61);
    outb(0x61, (uint8_t)(p | 3));
    timer_wait_ms(ms);
    outb(0x61, p);
}
