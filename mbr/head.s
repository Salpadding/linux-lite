    .code16
    .section .entry

.globl _start
_start:
    cli
    xor  %ax, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss
    movw $0xffff, %ax
    movw %ax, %sp
    movw %ax, %bp

    jmp  $0, $main


