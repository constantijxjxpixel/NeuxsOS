#include "pmm.h"
#include <stdint.h>

static inline void outb(uint16_t p, uint8_t v){ __asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p)); }
static inline uint8_t inb(uint16_t p){ uint8_t r; __asm__ volatile("inb %1,%0":"=a"(r):"Nd"(p)); return r; }

#define PAGE_SIZE  4096
#define PD_ADDR    0x100000
#define BITMAP     0x101000
#define RESERVED   0x104000
#define HEAP_START 0x200000
#define HEAP_SIZE  0x20000
#define PROG_BASE  0x400000
#define PROG_SIZE  0x100000

static uint8_t *bitmap = (uint8_t*)BITMAP;
static uint32_t total_pages = 0;
static uint32_t free_pages = 0;

uint32_t detect_ram_bytes(void) {
    outb(0x70, 0x17); uint8_t lo = inb(0x71);
    outb(0x70, 0x18); uint8_t hi = inb(0x71);
    uint32_t ext_kb = ((uint32_t)hi << 8) | lo;
    return (1024 + ext_kb) * 1024;
}

void paging_init(void) {
    uint32_t *pd = (uint32_t*)PD_ADDR;
    for (int i = 0; i < 1024; i++) pd[i] = 0;
    for (int i = 0; i < 8; i++) pd[i] = (uint32_t)(i * 0x400000) | 0x83;
    __asm__ volatile("mov %0, %%cr3" :: "r"((uint32_t)PD_ADDR));
    uint32_t cr4; __asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= (1u << 4);
    __asm__ volatile("mov %0, %%cr4" :: "r"(cr4));
    uint32_t cr0; __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= (1u << 31);
    __asm__ volatile("mov %0, %%cr0" :: "r"(cr0));
}

void pmm_init(uint32_t total_bytes) {
    total_pages = total_bytes / PAGE_SIZE;
    if (total_pages > 65536) total_pages = 65536;
    uint32_t bytes = (total_pages + 7) / 8;
    for (uint32_t i = 0; i < bytes; i++) bitmap[i] = 0xFF;
    free_pages = 0;
    for (uint32_t p = 0; p < total_pages; p++) {
        uint32_t addr = p * PAGE_SIZE;
        int used = (addr < RESERVED) ||
                   (addr >= HEAP_START && addr < HEAP_START + HEAP_SIZE) ||
                   (addr >= PROG_BASE && addr < PROG_BASE + PROG_SIZE);
        if (!used) { bitmap[p/8] &= (uint8_t)~(1u << (p & 7)); free_pages++; }
    }
}

uint32_t pmm_alloc_page(void) {
    for (uint32_t p = 0; p < total_pages; p++) {
        if (!(bitmap[p/8] & (1u << (p & 7)))) {
            bitmap[p/8] |= (uint8_t)(1u << (p & 7));
            free_pages--;
            return p * PAGE_SIZE;
        }
    }
    return 0;
}

void pmm_free_page(uint32_t addr) {
    uint32_t p = addr / PAGE_SIZE;
    if (p < total_pages && (bitmap[p/8] & (1u << (p & 7)))) {
        bitmap[p/8] &= (uint8_t)~(1u << (p & 7));
        free_pages++;
    }
}

uint32_t pmm_total_pages(void) { return total_pages; }
uint32_t pmm_free_pages(void) { return free_pages; }
