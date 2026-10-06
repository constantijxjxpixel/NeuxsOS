#include "keyboard.h"
#include "idt.h"
#include "serial.h"
#include <stdint.h>
static inline uint8_t inb(uint16_t p){ uint8_t r; __asm__ volatile("inb %1,%0":"=a"(r):"Nd"(p)); return r; }
static inline void outb(uint16_t p, uint8_t v){ __asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p)); }

static char kbd_buffer[256];
static volatile int kbd_head = 0, kbd_tail = 0;
static uint8_t scan_buffer[256];
static volatile int scan_head = 0, scan_tail = 0;
static int shift_state = 0;

static const char sc_ascii[128] = {
    0, 27, '1','2','3','4','5','6','7','8','9','0','-','=', '\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0,'a','s','d','f','g','h','j','k','l',';','\'','`',
    0,'\\','z','x','c','v','b','n','m',',','.','/',0,
    '*',0,' '
};
static const char sc_ascii_shift[128] = {
    0, 27, '!','@','#','$','%','^','&','*','(',')','_','+', '\b',
    '\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n',
    0,'A','S','D','F','G','H','J','K','L',':','"','~',
    0,'|','Z','X','C','V','B','N','M','<','>','?',0,
    '*',0,' '
};

extern void irq1_handler(void);

void keyboard_handler(void) {
    uint8_t sc = inb(0x60);
    outb(0x20, 0x20);
    scan_buffer[scan_head] = sc;
    scan_head = (scan_head + 1) & 255;
    if (sc == 0x2A || sc == 0x36) { shift_state = 1; return; }
    if (sc == 0xAA || sc == 0xB6) { shift_state = 0; return; }
    if (sc & 0x80) return;
    char c = shift_state ? sc_ascii_shift[sc] : sc_ascii[sc];
    if (c) { kbd_buffer[kbd_head] = c; kbd_head = (kbd_head + 1) & 255; }
}

int keyboard_kbhit(void) { return kbd_head != kbd_tail; }
char keyboard_getchar(void) {
    while (kbd_head == kbd_tail) __asm__ volatile("hlt");
    char c = kbd_buffer[kbd_tail];
    kbd_tail = (kbd_tail + 1) & 255;
    return c;
}
uint8_t keyboard_getscan(void) {
    while (scan_head == scan_tail) __asm__ volatile("hlt");
    uint8_t v = scan_buffer[scan_tail];
    scan_tail = (scan_tail + 1) & 255;
    return v;
}
int keyboard_scan_pending(void) { return scan_head != scan_tail; }
void keyboard_flush(void) { kbd_head = kbd_tail = 0; scan_head = scan_tail = 0; }
char keyboard_ascii(uint8_t sc) {
    sc &= 0x7F;
    return shift_state ? sc_ascii_shift[sc] : sc_ascii[sc];
}
void keyboard_install(void) {
    idt_set_gate(33, (uint32_t)irq1_handler, 0x08, 0x8E);
    outb(0x21, inb(0x21) & 0xFD);
    serial_puts("[KEYBOARD] PS/2 driver installed (IRQ1)\n");
}
