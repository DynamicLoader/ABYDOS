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

#include <stdio.h>
// #include <threads.h>

#include "k_defs.h"
#include "llenv.h"
#include "libfdt.h"

void *k_fdt = NULL;

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

unsigned long k_heap_max = 0;
// extern void *end;

void *_sbrk(ptrdiff_t incr)
{
    // _write(1, "_sbrk called\n", 14);

    extern char end asm("end"); /* Defined by the linker.  */
    static char *heap_end;
    char *prev_heap_end;

    if (heap_end == NULL)
        heap_end = &end;

    prev_heap_end = heap_end;

    // if ((heap_end + incr > stack_ptr)
    //     /* Honour heap limit if it's valid.  */
    //     || (__heap_limit != 0xcafedead && heap_end + incr > (char *)__heap_limit))
    // {
    //     extern errno = ENOMEM;
    //     return (void *)-1;
    // }

    heap_end += incr;
    if (heap_end - &end > k_heap_max)
        k_heap_max = heap_end - &end;

    return (void *)prev_heap_end;
}

// kernel init
int k_early_boot(const void *fdt)
{
    printf("\n===== Entered Test Kernel =====\n");

    // Copy fdt to heap
    k_fdt = malloc(fdt_totalsize(fdt));
    if (!k_fdt)
    {
        printf("Failed to allocate memory for FDT\n");
        return K_ENOMEM;
    }
    memcpy(k_fdt, fdt, fdt_totalsize(fdt));

    // init C++ exceptions
    extern void *__eh_frame_start;
    extern void __register_frame(void *); // not knowing the prototype of __register_frame, guess and OK!
    __register_frame(&__eh_frame_start);

    // Call global constructors
    printf("Calling init_array...\n");
    typedef void (*__init_func_ptr)();
    extern __init_func_ptr _init_array_start[0], _init_array_end[0];
    for (__init_func_ptr *func = _init_array_start; func != _init_array_end; func++)
        (*func)();

    return 0;
}

// kernel exit
void k_clearup(int main_ret)
{
    // k_stdout_switched = false; // switching back to default stdout
    printf("\nReached k_after_main, clearing up...\n");

    extern void __cxa_finalize(void *f); // in k_cxxabi.h
    __cxa_finalize(NULL);

    typedef void (*__init_func_ptr)();
    extern __init_func_ptr _fini_array_start[0], _fini_array_end[0];
    for (__init_func_ptr *func = _fini_array_start; func != _fini_array_end; func++)
        (*func)();

    extern char end asm("end");
    printf("* Kernel heap usage: %li\n", k_heap_max);
    printf("===== Test Kernel exited with %i =====\n", main_ret);
}

// void *k_getHartLocal()
// {
//     extern uintptr_t _sys_stack_base;
//     extern char _KERNEL_HART_LOCAL_DATA_SIZE;
//     return (void *)(_sys_stack_base - (uintptr_t)&_KERNEL_HART_LOCAL_DATA_SIZE);
// }