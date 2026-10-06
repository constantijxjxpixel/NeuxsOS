.section .text.boot
.global _start
.type _start, @function
_start:
    mov $stack_top, %esp

    # ??????? BSS: ???????? ?????? ?? _edata ?? _end
    mov $_edata, %edi
    mov $_end, %ecx
    sub %edi, %ecx
    xor %eax, %eax
    rep stosb

    push %ebx
    push %eax
    call kernel_main
1:  cli
    hlt
    jmp 1b
.size _start, . - _start

.section .bss
.align 16
stack_bottom:
.skip 16384
stack_top:
