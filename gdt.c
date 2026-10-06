#include "gdt.h"
#include "serial.h"
#include <stdint.h>

static struct gdt_entry gdt[6];
static struct gdt_ptr   gp;

extern void gdt_flush(uint32_t);

void gdt_install(void) {
    serial_puts("[DBG] gdt: start\n");
    gp.limit = (sizeof(struct gdt_entry) * 6) - 1;
    gp.base  = (uint32_t)&gdt;
    
    serial_puts("[DBG] gdt: gate 0\n");
    gdt[0].base_low=0; gdt[0].base_middle=0; gdt[0].base_high=0;
    gdt[0].limit_low=0; gdt[0].granularity=0; gdt[0].access=0;
    
    serial_puts("[DBG] gdt: gate 1\n");
    gdt[1].base_low=0; gdt[1].base_middle=0; gdt[1].base_high=0;
    gdt[1].limit_low=0xFFFF; gdt[1].granularity=0xCF; gdt[1].access=0x9A;
    
    serial_puts("[DBG] gdt: gate 2\n");
    gdt[2].base_low=0; gdt[2].base_middle=0; gdt[2].base_high=0;
    gdt[2].limit_low=0xFFFF; gdt[2].granularity=0xCF; gdt[2].access=0x92;
    
    serial_puts("[DBG] gdt: gate 3\n");
    gdt[3].base_low=0; gdt[3].base_middle=0; gdt[3].base_high=0;
    gdt[3].limit_low=0xFFFF; gdt[3].granularity=0xCF; gdt[3].access=0xFA;
    
    serial_puts("[DBG] gdt: gate 4\n");
    gdt[4].base_low=0; gdt[4].base_middle=0; gdt[4].base_high=0;
    gdt[4].limit_low=0xFFFF; gdt[4].granularity=0xCF; gdt[4].access=0xF2;
    
    serial_puts("[DBG] gdt: gate 5\n");
    gdt[5].base_low=0; gdt[5].base_middle=0; gdt[5].base_high=0;
    gdt[5].limit_low=0xFFFF; gdt[5].granularity=0xCF; gdt[5].access=0x9A;
    
    serial_puts("[DBG] gdt: lgdt\n");
    gdt_flush((uint32_t)&gp);
    serial_puts("[DBG] gdt: done\n");
}
