#include "mouse.h"
#include "idt.h"
#include "serial.h"
#include <stdint.h>
static inline void outb(uint16_t p, uint8_t v){ __asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p)); }
static inline uint8_t inb(uint16_t p){ uint8_t r; __asm__ volatile("inb %1,%0":"=a"(r):"Nd"(p)); return r; }

static volatile int m_x=40, m_y=12, m_btn=0;
static int cyc=0;
static uint8_t pk[3];

extern void irq12_handler(void);

void mouse_handler(void){
    uint8_t v = inb(0x60);
    pk[cyc++] = v;
    if (cyc >= 3) {
        cyc = 0;
        int dx = (int)(int8_t)pk[1];
        int dy = (int)(int8_t)pk[2];
        m_btn = pk[0] & 0x07;
        m_x += dx; m_y -= dy;
        if (m_x < 0) m_x = 0; if (m_x > 79) m_x = 79;
        if (m_y < 0) m_y = 0; if (m_y > 24) m_y = 24;
    }
    outb(0xA0, 0x20);
    outb(0x20, 0x20);
}

void mouse_get(int *x, int *y, int *btn){ *x=m_x; *y=m_y; *btn=m_btn; }

static void wait_in(void){ while(!(inb(0x64)&1)); }
static void wait_wr(void){ while(inb(0x64)&2); }

void mouse_install(void){
    wait_wr(); outb(0x64, 0xA8);
    wait_wr(); outb(0x64, 0xD4); outb(0x60, 0xF6); wait_in(); (void)inb(0x60);
    wait_wr(); outb(0x64, 0xD4); outb(0x60, 0xF4); wait_in(); (void)inb(0x60);
    idt_set_gate(44, (uint32_t)irq12_handler, 0x08, 0x8E);
    outb(0x21, (uint8_t)(inb(0x21) & 0xFB));
    outb(0xA1, (uint8_t)(inb(0xA1) & 0xEF));
    serial_puts("[MOUSE] PS/2 mouse installed (IRQ12)\n");
}
