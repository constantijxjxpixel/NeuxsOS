#include <stdint.h>
#include "serial.h"
#include "gdt.h"
#include "idt.h"
#include "irq.h"
#include "timer.h"
#include "pmm.h"
#include "kmalloc.h"
#include "vga.h"
#include "keyboard.h"
#include "mouse.h"
#include "speaker.h"
#include "splash.h"
#include "fs.h"
#include "sched.h"
#include "task.h"
#include "elf.h"
#include "net.h"
#include "shell.h"

void kernel_main(uint32_t mb_magic, uint32_t mb_info) {
    (void)mb_info; (void)mb_magic;
    serial_init();
    gdt_install();
    idt_install();
    isrs_install();
    irq_install();
    elf_install();
    vga_init();
    keyboard_install();
    timer_install();

    uint32_t ram = detect_ram_bytes();
    paging_init();
    pmm_init(ram);
    kmalloc_init();
    fs_init();
    if (fs_load()) serial_puts("[STAGE9] fs restored from disk\n");
    else serial_puts("[STAGE9] fs fresh\n");
    mouse_install();
    net_install();

    __asm__ volatile("sti");
    serial_puts("[STAGE9] interrupts enabled\n");

    splash_show(); vga_clear();
    speaker_beep(523,120);
    speaker_beep(659,120);
    speaker_beep(784,200);

    sched_install();
    task_spawn("shell", shell_run);
    serial_puts("[STAGE9] scheduler on\n");

    for(;;) __asm__ volatile("hlt");
}

