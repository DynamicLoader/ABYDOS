
#include "syscall.h"

    .section .umode, "ax", %progbits
	.align 3
	.globl _umode_start,_umode_end
    .option norelax
_umode_start:
    li a0, SYSCALL_PRINTF
    lla a1, _umode_data // Always use lla to avoid GP-relative addressing
    li a2, 114514
    ecall

1:    j 1b

_umode_data:
    .string "Hello, World! %i\n"

    .align 3
_umode_end:

    