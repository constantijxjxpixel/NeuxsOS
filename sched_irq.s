.text
.global irq0_handler
irq0_handler:
    push $0
    push $32
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
    mov %esp, %eax
    push $1
    push %eax
    call scheduler_tick
    add $8, %esp
    mov %eax, %esp
    pop %gs
    pop %fs
    pop %es
    pop %ds
    popa
    add $8, %esp
    iret

.global task_yield
task_yield:
    cli
    push $0
    push $32
    pusha
    push %ds
    push %es
    push %fs
    push %gs
    mov %esp, %eax
    push $0
    push %eax
    call scheduler_tick
    add $8, %esp
    mov %eax, %esp
    pop %gs
    pop %fs
    pop %es
    pop %ds
    popa
    add $8, %esp
    sti
    ret
