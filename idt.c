#include "idt.h"
#include "serial.h"
#include <stdint.h>

static struct idt_entry idt[256];

extern void idt_flush(uint32_t);

void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt[num].base_lo = base & 0xFFFF;
    idt[num].base_hi = (base >> 16) & 0xFFFF;
    idt[num].sel     = sel;
    idt[num].always0 = 0;
    idt[num].flags   = flags;
}

void idt_install(void) {
    struct idt_ptr ip;
    serial_puts("[DBG] idt: start\n");
    ip.limit = sizeof(struct idt_entry) * 256 - 1;
    ip.base  = (uint32_t)&idt;
    serial_puts("[DBG] idt: pic remap\n");
    __asm__ volatile("outb %0, %1" : : "a"((uint8_t)0x11), "Nd"((uint16_t)0x20));
    __asm__ volatile("outb %0, %1" : : "a"((uint8_t)0x11), "Nd"((uint16_t)0xA0));
    __asm__ volatile("outb %0, %1" : : "a"((uint8_t)0x20), "Nd"((uint16_t)0x21));
    __asm__ volatile("outb %0, %1" : : "a"((uint8_t)0x28), "Nd"((uint16_t)0xA1));
    __asm__ volatile("outb %0, %1" : : "a"((uint8_t)0x04), "Nd"((uint16_t)0x21));
    __asm__ volatile("outb %0, %1" : : "a"((uint8_t)0x02), "Nd"((uint16_t)0xA1));
    __asm__ volatile("outb %0, %1" : : "a"((uint8_t)0x01), "Nd"((uint16_t)0x21));
    __asm__ volatile("outb %0, %1" : : "a"((uint8_t)0x01), "Nd"((uint16_t)0xA1));
    __asm__ volatile("outb %0, %1" : : "a"((uint8_t)0xFF), "Nd"((uint16_t)0x21));
    __asm__ volatile("outb %0, %1" : : "a"((uint8_t)0xFF), "Nd"((uint16_t)0xA1));
    serial_puts("[DBG] idt: lidt\n");
    idt_flush((uint32_t)&ip);
    serial_puts("[DBG] idt: done\n");
}
