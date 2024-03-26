
#ifndef __LLENV_H__
#define __LLENV_H__

#include <sbi/sbi_ecall_interface.h>
#include <sbi/riscv_encoding.h>
#include "riscv_asm.h"
#include <stdint.h>
#include <reent.h>

#include "k_defs.h"
#include <sys/cdefs.h>

struct sbiret
{
    long error;
    long value;
};

__always_inline struct sbiret sbi_ecall(int ext, int fid, unsigned long arg0, unsigned long arg1, unsigned long arg2,
                                        unsigned long arg3, unsigned long arg4, unsigned long arg5)

{
    struct sbiret ret;

    register unsigned long a0 asm("a0") = (unsigned long)(arg0);
    register unsigned long a1 asm("a1") = (unsigned long)(arg1);
    register unsigned long a2 asm("a2") = (unsigned long)(arg2);
    register unsigned long a3 asm("a3") = (unsigned long)(arg3);
    register unsigned long a4 asm("a4") = (unsigned long)(arg4);
    register unsigned long a5 asm("a5") = (unsigned long)(arg5);
    register unsigned long a6 asm("a6") = (unsigned long)(fid);
    register unsigned long a7 asm("a7") = (unsigned long)(ext);
    asm volatile("ecall" : "+r"(a0), "+r"(a1) : "r"(a2), "r"(a3), "r"(a4), "r"(a5), "r"(a6), "r"(a7) : "memory");
    ret.error = a0;
    ret.value = a1;

    return ret;
}

// extern uint8_t k_fdt[K_FDT_MAX_SIZE];
extern void *k_fdt;

// void *k_getHartLocal();

#endif