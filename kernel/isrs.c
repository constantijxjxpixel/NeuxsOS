#include "idt.h"
#include "serial.h"
#include "vga.h"
#include <stdint.h>

extern void isr0();  extern void isr1();  extern void isr2();  extern void isr3();
extern void isr4();  extern void isr5();  extern void isr6();  extern void isr7();
extern void isr8();  extern void isr9();  extern void isr10(); extern void isr11();
extern void isr12(); extern void isr13(); extern void isr14(); extern void isr15();
extern void isr16(); extern void isr17(); extern void isr18(); extern void isr19();
extern void isr20(); extern void isr21(); extern void isr22(); extern void isr23();
extern void isr24(); extern void isr25(); extern void isr26(); extern void isr27();
extern void isr28(); extern void isr29(); extern void isr30(); extern void isr31();

static const char *exception_messages[32] = {
    "Division By Zero","Debug","Non Maskable Interrupt","Breakpoint",
    "Into Detected Overflow","Out of Bounds","Invalid Opcode","No Coprocessor",
    "Double Fault","Coprocessor Segment Overrun","Bad TSS","Segment Not Present",
    "Stack Fault","General Protection Fault","Page Fault","Unknown Interrupt",
    "Coprocessor Fault","Alignment Check","Machine Check","Reserved",
    "Reserved","Reserved","Reserved","Reserved","Reserved","Reserved","Reserved","Reserved",
    "Reserved","Reserved","Reserved","Reserved"
};

struct regs { uint32_t gs,fs,es,ds; uint32_t edi,esi,ebp,esp,ebx,edx,ecx,eax; uint32_t int_no,err_code; uint32_t eip,cs,eflags,useresp,ss; };

static void p_at(int x,int y,const char *s){
    for(int i=0;s[i];i++) vga_putc_at(x+i,y,s[i],VGA_WHITE,VGA_RED);
}
static void p_hex(int x,int y,uint32_t v){
    const char *d="0123456789ABCDEF";
    char b[9]; for(int i=7;i>=0;i--){ b[i]=d[v&0xF]; v>>=4; }
    b[8]=0; p_at(x,y,b);
}

void fault_handler(struct regs *r) {
    if (r->int_no < 32) {
        for(int y=0;y<25;y++) for(int x=0;x<80;x++) vga_putc_at(x,y,' ',VGA_WHITE,VGA_RED);
        p_at(2,2,"*** NEXUSOS PANIC ***");
        p_at(2,4,exception_messages[r->int_no]);
        p_at(2,6,"EIP="); p_hex(6,6,r->eip);
        p_at(2,7,"ESP="); p_hex(6,7,r->esp);
        p_at(2,8,"EAX="); p_hex(6,8,r->eax);
        p_at(2,9,"ERR="); p_hex(6,9,r->err_code);
        p_at(2,11,"System halted. Reboot the VM.");
        serial_puts("\n[PANIC] ");
        serial_puts(exception_messages[r->int_no]);
        serial_puts("\n");
        for (;;) __asm__ volatile("cli; hlt");
    }
}

void isrs_install(void) {
    idt_set_gate(0,(uint32_t)isr0,0x08,0x8E);  idt_set_gate(1,(uint32_t)isr1,0x08,0x8E);
    idt_set_gate(2,(uint32_t)isr2,0x08,0x8E);  idt_set_gate(3,(uint32_t)isr3,0x08,0x8E);
    idt_set_gate(4,(uint32_t)isr4,0x08,0x8E);  idt_set_gate(5,(uint32_t)isr5,0x08,0x8E);
    idt_set_gate(6,(uint32_t)isr6,0x08,0x8E);  idt_set_gate(7,(uint32_t)isr7,0x08,0x8E);
    idt_set_gate(8,(uint32_t)isr8,0x08,0x8E);  idt_set_gate(9,(uint32_t)isr9,0x08,0x8E);
    idt_set_gate(10,(uint32_t)isr10,0x08,0x8E);idt_set_gate(11,(uint32_t)isr11,0x08,0x8E);
    idt_set_gate(12,(uint32_t)isr12,0x08,0x8E);idt_set_gate(13,(uint32_t)isr13,0x08,0x8E);
    idt_set_gate(14,(uint32_t)isr14,0x08,0x8E);idt_set_gate(15,(uint32_t)isr15,0x08,0x8E);
    idt_set_gate(16,(uint32_t)isr16,0x08,0x8E);idt_set_gate(17,(uint32_t)isr17,0x08,0x8E);
    idt_set_gate(18,(uint32_t)isr18,0x08,0x8E);idt_set_gate(19,(uint32_t)isr19,0x08,0x8E);
    idt_set_gate(20,(uint32_t)isr20,0x08,0x8E);idt_set_gate(21,(uint32_t)isr21,0x08,0x8E);
    idt_set_gate(22,(uint32_t)isr22,0x08,0x8E);idt_set_gate(23,(uint32_t)isr23,0x08,0x8E);
    idt_set_gate(24,(uint32_t)isr24,0x08,0x8E);idt_set_gate(25,(uint32_t)isr25,0x08,0x8E);
    idt_set_gate(26,(uint32_t)isr26,0x08,0x8E);idt_set_gate(27,(uint32_t)isr27,0x08,0x8E);
    idt_set_gate(28,(uint32_t)isr28,0x08,0x8E);idt_set_gate(29,(uint32_t)isr29,0x08,0x8E);
    idt_set_gate(30,(uint32_t)isr30,0x08,0x8E);idt_set_gate(31,(uint32_t)isr31,0x08,0x8E);
}
