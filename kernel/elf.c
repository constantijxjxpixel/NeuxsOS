#include "elf.h"
#include "ata.h"
#include "idt.h"
#include "vga.h"
#include "serial.h"
#include "speaker.h"
#include "timer.h"
#include "task.h"
#include <stdint.h>

static uint8_t elf_buf[32768];

extern void isr128_handler(void);

struct elf_hdr {
    uint8_t ident[16];
    uint16_t type, machine;
    uint32_t version, entry, phoff, shoff;
    uint32_t flags;
    uint16_t ehsize, phentsize, phnum, shentsize, shnum, shstrndx;
} __attribute__((packed));

struct phdr { uint32_t type, offset, vaddr, paddr, filesz, memsz, flags, align; } __attribute__((packed));

struct regs { uint32_t gs,fs,es,ds; uint32_t edi,esi,ebp,esp,ebx,edx,ecx,eax; uint32_t int_no,err_code; uint32_t eip,cs,eflags,useresp,ss; };

static void kputs(const char *s){ vga_puts(s); serial_puts(s); }

void elf_install(void){ idt_set_gate(128, (uint32_t)isr128_handler, 0x08, 0x8E); }

uint32_t elf_load_slot(int slot){
    ata_read_sectors((uint32_t)(4096 + slot*64), 64, elf_buf);
    struct elf_hdr *h = (struct elf_hdr*)elf_buf;
    if (h->ident[0]!=0x7F || h->ident[1]!='E' || h->ident[2]!='L' || h->ident[3]!='F') return 0;
    if (h->ident[4]!=1) return 0;
    struct phdr *ph = (struct phdr*)(elf_buf + h->phoff);
    for (int i=0;i<h->phnum;i++){
        if (ph[i].type==1){
            if (ph[i].offset + ph[i].filesz > sizeof(elf_buf)) return 0;
            uint8_t *dst=(uint8_t*)(uint32_t)ph[i].paddr;
            uint8_t *src=elf_buf+ph[i].offset;
            for (uint32_t k=0;k<ph[i].filesz;k++) dst[k]=src[k];
            for (uint32_t k=ph[i].filesz;k<ph[i].memsz;k++) dst[k]=0;
        }
    }
    return h->entry;
}

uint32_t syscall_handler(struct regs *r){
    switch (r->eax){
        case 1: kputs((const char*)r->ebx); return 0;
        case 2: task_exit(); return 0;
        case 3: speaker_beep(r->ebx,150); return 0;
        case 4: return timer_get_ticks();
        default: return 0xFFFFFFFF;
    }
}
