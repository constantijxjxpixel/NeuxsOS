.text
.global isr128_handler
isr128_handler:
    push $0
    push $128
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
    call syscall_handler
    mov 4(%esp), %edx
    mov %eax, 44(%edx)
    add $4, %esp
    pop %gs
    pop %fs
    pop %es
    pop %ds
    popa
    add $8, %esp
    iret
