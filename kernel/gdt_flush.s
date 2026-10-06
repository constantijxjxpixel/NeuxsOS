.section .text
.global gdt_flush
gdt_flush:
    mov 4(%esp), %eax
    lgdt (%eax)
    ret
