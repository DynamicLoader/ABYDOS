
#include "syscall.h"

    .section .umode1, "ax", %progbits
	.align 3
	.globl _umode1_start,_umode1_end
    .option norelax
_umode1_start:
    li a0, SYSCALL_PRINTF
    lla a1, _umode1_data // Always use lla to avoid GP-relative addressing
    li a2, 114514
    ecall

1:    j 1b

_umode1_data:
    .string "Hello, World! %i\n"

    .align 3
_umode1_end:

    