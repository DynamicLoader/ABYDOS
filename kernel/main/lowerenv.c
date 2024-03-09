/**
 * @file k_lowerenv.c
 * @author DynamicLoader (admin@dyldr.top)
 * @brief Lower environment for kernel in C
 * @version 0.1
 * @date 2024-03-08
 *
 * @copyright Copyright (c) 2024
    @ref https://wiki.osdev.org/Calling_Global_Constructors
    https://wiki.osdev.org/Libsupcxx#Full_C.2B.2B_Runtime_Support_Using_libgcc_And_libsupc.2B.2B
 *
 */

#include <string.h>
#include <stdio.h>

#include "k_main.h"
#include "llenv.h"

const char default_args[] = "--test -tty serial";
const char *kernel_cmdargs[128] = {0};
int kernel_cmdargc = 0;

typedef void (*__init_func_ptr)();
extern __init_func_ptr _init_array_start[0], _init_array_end[0];
extern __init_func_ptr _fini_array_start[0], _fini_array_end[0];

void _init(void)
{
}

void _fini(void)
{
}

struct sbiret sbi_ecall(int ext, int fid, unsigned long arg0, unsigned long arg1, unsigned long arg2,
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

// Hook with libc
int _write(int fd, char *buf, int size)
{
    sbi_ecall(SBI_EXT_DBCN, SBI_EXT_DBCN_CONSOLE_WRITE, size, (unsigned long)buf, 0, 0, 0, 0);
    return size;
}

static int split_args(char *in, const char **out) // Out of buffer to be fixed
{
    int in_quotes = 0;
    int count = 0;
    out[count] = in;

    for (; *in; ++in)
    {
        if (*in == '\"')
        {
            in_quotes = !in_quotes;
            continue;
        }
        if (*in == ' ' && !in_quotes)
        {
            *in = '\0';
            out[++(count)] = in + 1;
        }
    }
    (count)++; // always return +1
    return count;
}

// kernel init
void k_before_main(unsigned long *pa0, unsigned long *pa1)
{
    printf("\n===== Entered Test Kernel =====\n");
    printf("a0: 0x%lx \t a1: 0x%lx\n", *pa0, *pa1);

    printf("Calling init_array...\n");
    for (__init_func_ptr *func = _init_array_start; func != _init_array_end; func++)
        (*func)();

    // No args, use default
    if (*pa0 == 0)
        *pa0 = (unsigned long)default_args;
}

// kernel exit
void k_after_main(int main_ret)
{
    printf("\nReached target k_after_main, clearing up...\n");

    for (__init_func_ptr *func = _fini_array_start; func != _fini_array_end; func++)
        (*func)();

    printf("===== Test Kernel exited with %i =====\n", main_ret);
}

// Prepare C++ environment
void k_prep_cxx()
{
    // init C++ exceptions
    extern void *__eh_frame_start;
    extern void __register_frame(void *); // not knowing the prototype of __register_frame, guess and OK!
    __register_frame(&__eh_frame_start);
}

void k_cstart(unsigned long a0, unsigned long a1) // To be called from prepC.S
{
    k_before_main(&a0, &a1);

    // Parse Kerenl command line args
    kernel_cmdargc = split_args((char *)a0, kernel_cmdargs);

    // init C++ exceptions
    k_prep_cxx();

    int ret = k_main(kernel_cmdargc, kernel_cmdargs);

    k_after_main(ret);
    // After exit from here, infini loop in asm...
}
