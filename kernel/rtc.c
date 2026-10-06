#include "rtc.h"
#include <stdint.h>

static inline void outb(uint16_t p, uint8_t v){ __asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p)); }
static inline uint8_t inb(uint16_t p){ uint8_t r; __asm__ volatile("inb %1,%0":"=a"(r):"Nd"(p)); return r; }

static uint8_t cmos_read(uint8_t reg) {
    outb(0x70, reg);
    return inb(0x71);
}

static uint8_t bcd_to_bin(uint8_t v) {
    return (v & 0x0F) + ((v >> 4) * 10);
}

void rtc_read(struct rtc_time *t) {
    while (!(cmos_read(0x0A) & 0x80)) { }
    while (cmos_read(0x0A) & 0x80) { }
    uint8_t sec  = cmos_read(0x00);
    uint8_t min  = cmos_read(0x02);
    uint8_t hour = cmos_read(0x04);
    uint8_t day  = cmos_read(0x07);
    uint8_t mon  = cmos_read(0x08);
    uint8_t year = cmos_read(0x09);
    uint8_t sb   = cmos_read(0x0B);

    uint8_t pm = hour & 0x80;
    hour &= 0x7F;

    if (!(sb & 0x04)) {
        sec  = bcd_to_bin(sec);
        min  = bcd_to_bin(min);
        hour = bcd_to_bin(hour);
        day  = bcd_to_bin(day);
        mon  = bcd_to_bin(mon);
        year = bcd_to_bin(year);
    }
    if (!(sb & 0x02)) {
        if (pm) hour = (hour % 12) + 12;
        else    hour = hour % 12;
    }
    t->second = sec; t->minute = min; t->hour = hour;
    t->day = day; t->month = mon; t->year = (uint16_t)year + 2000;
}
