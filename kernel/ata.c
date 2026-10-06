#include "ata.h"
#include <stdint.h>
static inline void outb(uint16_t p,uint8_t v){ __asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p)); }
static inline uint8_t inb(uint16_t p){ uint8_t r; __asm__ volatile("inb %1,%0":"=a"(r):"Nd"(p)); return r; }

static void io_wait(void){ for(int i=0;i<4;i++) inb(0x1F7); }
static void wait_bsy(void){ while(inb(0x1F7)&0x80); }
static void wait_drq(void){ while(!(inb(0x1F7)&0x08)); }

void ata_read_sectors(uint32_t lba, uint8_t count, uint8_t *buf){
    wait_bsy();
    outb(0x1F6, (uint8_t)(0xE0 | ((lba>>24)&0x0F)));
    outb(0x1F2, count);
    outb(0x1F3, (uint8_t)lba);
    outb(0x1F4, (uint8_t)(lba>>8));
    outb(0x1F5, (uint8_t)(lba>>16));
    outb(0x1F7, 0x20);
    for (uint8_t s=0;s<count;s++){
        wait_bsy(); wait_drq();
        uint32_t n=256;
        __asm__ volatile("rep insw" : "+D"(buf), "+c"(n) : "d"((uint16_t)0x1F0) : "memory");
        buf += 512;
        io_wait();
    }
}

void ata_write_sectors(uint32_t lba, uint8_t count, const uint8_t *buf){
    wait_bsy();
    outb(0x1F6, (uint8_t)(0xE0 | ((lba>>24)&0x0F)));
    outb(0x1F2, count);
    outb(0x1F3, (uint8_t)lba);
    outb(0x1F4, (uint8_t)(lba>>8));
    outb(0x1F5, (uint8_t)(lba>>16));
    outb(0x1F7, 0x30);
    for (uint8_t s=0;s<count;s++){
        wait_bsy(); wait_drq();
        uint32_t n=256;
        uint8_t *b = (uint8_t*)buf;
        __asm__ volatile("rep outsw" : "+S"(b), "+c"(n) : "d"((uint16_t)0x1F0) : "memory");
        buf = b + 512;
        io_wait();
    }
    wait_bsy();
}
