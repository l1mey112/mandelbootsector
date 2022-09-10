[BITS 16]
section .text

global mystart
extern __stack_top
extern main

xor ax, ax
mov es, ax
mov ds, ax
mov ss, ax
mov ds, ax
mov sp, __stack_top
cld
call main
__hlt:
hlt
jmp __hlt