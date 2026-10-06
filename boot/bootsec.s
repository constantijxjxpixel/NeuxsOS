.code16
.text
.global _start
_start:
    mov $0x3FB, %dx
    mov $0x80, %al
    out %al, %dx
    mov $0x3F8, %dx
    mov $0x01, %al
    out %al, %dx
    mov $0x3F9, %dx
    xor %al, %al
    out %al, %dx
    mov $0x3FB, %dx
    mov $0x03, %al
    out %al, %dx
    mov $0x3FA, %dx
    mov $0xC7, %al
    out %al, %dx
    mov $0x3FC, %dx
    mov $0x0B, %al
    out %al, %dx
    mov $0x42, %al
    call ser_put
    cli
    xor %ax, %ax
    mov %ax, %ss
    mov $0x7C00, %sp
    mov %ax, %ds
    mov %ax, %es
    mov $0x1000, %ax
    mov %ax, %es
    xor %bx, %bx
    mov $0x7E00, %si
    movw $0x10, 0(%si)
    movw $126, 2(%si)
    movw $0x0000, 4(%si)
    movw $0x1000, 6(%si)
    movl $1, 8(%si)
    movl $0, 12(%si)
    mov $0x4200, %ax
    mov $0x80, %dl
    int $0x13
    jc disk_error
    mov $0x52, %al
    call ser_put
    lgdt gdt_desc
    mov %cr0, %eax
    or $1, %eax
    mov %eax, %cr0
    .byte 0x66, 0xea
    .long pm32
    .word 0x08
disk_error:
    mov $0x44, %al
    call ser_put
    jmp hang
ser_put:
    push %dx
    push %ax
    mov $0x3FD, %dx
1:  in %dx, %al
    test $0x20, %al
    jz 1b
    pop %ax
    mov $0x3F8, %dx
    out %al, %dx
    pop %dx
    ret
.code32
pm32:
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %ss
    mov %ax, %fs
    mov %ax, %gs
    mov $0x10000, %ecx
    jmp *%ecx
hang:
    cli
    hlt
    jmp hang
.p2align 4
gdt:
    .quad 0
    .quad 0x00CF9A000000FFFF
    .quad 0x00CF92000000FFFF
gdt_desc:
    .short 23
    .long gdt
    .fill 510 - (. - _start), 1, 0
    .byte 0x55, 0xAA
