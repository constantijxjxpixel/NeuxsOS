#include "timer.h"
#include <stdint.h>
static inline void outb(uint16_t p, uint8_t v){ __asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p)); }
static volatile uint32_t ticks = 0;
void timer_interrupt(void) { ticks++; }
uint32_t timer_get_ticks(void) { return ticks; }
uint32_t timer_get_seconds(void) { return ticks / 100; }
void timer_install(void) {
    outb(0x43, 0x36);
    outb(0x40, (uint8_t)(11932 & 0xFF));
    outb(0x40, (uint8_t)(11932 >> 8));
    uint8_t mask; __asm__ volatile("inb %1,%0":"=a"(mask):"Nd"((uint16_t)0x21));
    outb(0x21, mask & 0xFE);
}
void timer_sleep(uint32_t seconds) {
    uint32_t target = ticks + seconds * 100;
    while (ticks < target) __asm__ volatile("hlt");
}
void timer_wait_ms(uint32_t ms) {
    uint32_t t0 = ticks, need = ms / 10;
    while (ticks - t0 < need) __asm__ volatile("hlt");
}
