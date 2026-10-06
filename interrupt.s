.text
.global isr0
isr0:   push $0; push $0;  jmp isr_common_stub
.global isr1
isr1:   push $0; push $1;  jmp isr_common_stub
.global isr2
isr2:   push $0; push $2;  jmp isr_common_stub
.global isr3
isr3:   push $0; push $3;  jmp isr_common_stub
.global isr4
isr4:   push $0; push $4;  jmp isr_common_stub
.global isr5
isr5:   push $0; push $5;  jmp isr_common_stub
.global isr6
isr6:   push $0; push $6;  jmp isr_common_stub
.global isr7
isr7:   push $0; push $7;  jmp isr_common_stub
.global isr8
isr8:   push $8;  jmp isr_common_stub
.global isr9
isr9:   push $0; push $9;  jmp isr_common_stub
.global isr10
isr10:  push $10; jmp isr_common_stub
.global isr11
isr11:  push $11; jmp isr_common_stub
.global isr12
isr12:  push $12; jmp isr_common_stub
.global isr13
isr13:  push $13; jmp isr_common_stub
.global isr14
isr14:  push $14; jmp isr_common_stub
.global isr15
isr15:  push $0; push $15; jmp isr_common_stub
.global isr16
isr16:  push $0; push $16; jmp isr_common_stub
.global isr17
isr17:  push $17; jmp isr_common_stub
.global isr18
isr18:  push $0; push $18; jmp isr_common_stub
.global isr19
isr19:  push $0; push $19; jmp isr_common_stub
.global isr20
isr20:  push $0; push $20; jmp isr_common_stub
.global isr21
isr21:  push $0; push $21; jmp isr_common_stub
.global isr22
isr22:  push $0; push $22; jmp isr_common_stub
.global isr23
isr23:  push $0; push $23; jmp isr_common_stub
.global isr24
isr24:  push $0; push $24; jmp isr_common_stub
.global isr25
isr25:  push $0; push $25; jmp isr_common_stub
.global isr26
isr26:  push $0; push $26; jmp isr_common_stub
.global isr27
isr27:  push $0; push $27; jmp isr_common_stub
.global isr28
isr28:  push $0; push $28; jmp isr_common_stub
.global isr29
isr29:  push $0; push $29; jmp isr_common_stub
.global isr30
isr30:  push $30; jmp isr_common_stub
.global isr31
isr31:  push $0; push $31; jmp isr_common_stub

.extern fault_handler
isr_common_stub:
    pusha
    push %ds
    push %es
    push %fs
    push %gs
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    push %esp
    call fault_handler
    add $4, %esp
    pop %gs
    pop %fs
    pop %es
    pop %ds
    popa
    add $8, %esp
    iret

.global irq0
irq0:   push $0; push $32; jmp irq_common_stub
.global irq1
irq1:   push $0; push $33; jmp irq_common_stub
.global irq2
irq2:   push $0; push $34; jmp irq_common_stub
.global irq3
irq3:   push $0; push $35; jmp irq_common_stub
.global irq4
irq4:   push $0; push $36; jmp irq_common_stub
.global irq5
irq5:   push $0; push $37; jmp irq_common_stub
.global irq6
irq6:   push $0; push $38; jmp irq_common_stub
.global irq7
irq7:   push $0; push $39; jmp irq_common_stub
.global irq8
irq8:   push $0; push $40; jmp irq_common_stub
.global irq9
irq9:   push $0; push $41; jmp irq_common_stub
.global irq10
irq10:  push $0; push $42; jmp irq_common_stub
.global irq11
irq11:  push $0; push $43; jmp irq_common_stub
.global irq12
irq12:  push $0; push $44; jmp irq_common_stub
.global irq13
irq13:  push $0; push $45; jmp irq_common_stub
.global irq14
irq14:  push $0; push $46; jmp irq_common_stub
.global irq15
irq15:  push $0; push $47; jmp irq_common_stub

.extern irq_handler
irq_common_stub:
    pusha
    push %ds
    push %es
    push %fs
    push %gs
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    push %esp
    call irq_handler
    add $4, %esp
    pop %gs
    pop %fs
    pop %es
    pop %ds
    popa
    add $8, %esp
    iret
