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

#include "k_main.h"
#include "llenv.h"

#include "libfdt.h"

char kernel_args[4096] = "--test -tty serial";
const char *kernel_args_array[256] = {0};
int kernel_cmdargc = 0;

uint8_t k_fdt[K_FDT_MAX_SIZE] = {0};
uint64_t k_mem_size = 0;

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

// void *_sbrk(ptrdiff_t incr)
// {
//     _write(1, "_sbrk called\n", 14);

//     extern char end asm("end"); /* Defined by the linker.  */
//     static char *heap_end;
//     char *prev_heap_end;

//     if (heap_end == NULL)
//         heap_end = &end;

//     prev_heap_end = heap_end;

//     // if ((heap_end + incr > stack_ptr)
//     //     /* Honour heap limit if it's valid.  */
//     //     || (__heap_limit != 0xcafedead && heap_end + incr > (char *)__heap_limit))
//     // {
//     //     extern errno = ENOMEM;
//     //     return (void *)-1;
//     // }

//     heap_end += incr;

//     return (void *)prev_heap_end;
// }

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
void k_before_main(unsigned long pa0)
{
    printf("\n===== Entered Test Kernel =====\n");
    printf("a0: 0x%lx\n", pa0);

    // Initialize driver list
    // extern void k_drv_init();
    // k_drv_init();

    // Call global constructors
    printf("Calling init_array...\n");
    typedef void (*__init_func_ptr)();
    extern __init_func_ptr _init_array_start[0], _init_array_end[0];
    for (__init_func_ptr *func = _init_array_start; func != _init_array_end; func++)
        (*func)();
}

// kernel exit
void k_after_main(int main_ret)
{
    printf("\nReached k_after_main, clearing up...\n");

    extern void __cxa_finalize(void *f); // in k_cxxabi.h
    __cxa_finalize(NULL);

    typedef void (*__init_func_ptr)();
    extern __init_func_ptr _fini_array_start[0], _fini_array_end[0];
    for (__init_func_ptr *func = _fini_array_start; func != _fini_array_end; func++)
        (*func)();

    // Cleanup driver list
    // extern void k_drv_deinit();
    // k_drv_deinit();

    printf("===== Test Kernel exited with %i =====\n", main_ret);
}

void k_cstart(unsigned long a0) // To be called from prepC.S
{
    k_before_main(a0);

    int ret = k_main(kernel_cmdargc, kernel_args_array);

    k_after_main(ret);
    // After exit from here, infini loop in asm...
}

#ifdef DEBUG

static char depth_set[32];

static void pretty_node(int depth)
{
    if (depth == 0)
        return;

    for (int i = 0; i < depth - 1; ++i)
        printf(depth_set[i] ? "|   " : "    ");

    printf(depth_set[depth - 1] ? "+-- " : "\\-- ");
}

static void pretty_prop(int depth)
{
    for (int i = 0; i < depth; ++i)
        printf(depth_set[i] ? "|   " : "    ");

    printf(depth_set[depth] ? "|  " : "   ");
}

static void print_node_prop(const void *fdt, int node, int depth)
{
    int prop;
    fdt_for_each_property_offset(prop, fdt, node)
    {
        if (depth >= 0)
            pretty_prop(depth);
        int size;
        const char *name;
        const char *value = fdt_getprop_by_offset(fdt, prop, &name, &size);

        bool is_str = !(size > 1 && value[0] == 0);
        if (is_str)
        {
            // Scan through value to see if printable
            for (int i = 0; i < size; ++i)
            {
                char c = value[i];
                if (i == size - 1)
                {
                    // Make sure null terminate
                    is_str = c == '\0';
                }
                else if ((c > 0 && c < 32) || c >= 127)
                {
                    is_str = false;
                    break;
                }
            }
        }

        if (is_str)
        {
            printf("[%s]: [%s]\n", name, value);
        }
        else
        {
            // printf("[%s]: <bytes>(%d)\n", name, size);
            printf("[%s]: <bytes>(%d) ", name, size);
            for (int i = 0; i < size; i++)
                printf("0x%02X ", value[i]);
            printf("\n");
        }
    }
}

static void print_node(const void *fdt, int node, int depth)
{
    // Print node itself
    pretty_node(depth);
    printf("#%d: %s\n", node, fdt_get_name(fdt, node, NULL));

    // Print properties
    depth_set[depth] = fdt_first_subnode(fdt, node) >= 0;
    print_node_prop(fdt, node, depth);

    // Recursive
    if (depth_set[depth])
    {
        int child;
        int prev = -1;
        fdt_for_each_subnode(child, fdt, node)
        {
            if (prev >= 0)
                print_node(fdt, prev, depth + 1);
            prev = child;
        }
        depth_set[depth] = false;
        print_node(fdt, prev, depth + 1);
    }
}

#endif

uint64_t be2le64(const uint64_t value)
{
    return ((value & 0xFF00000000000000) >> 56) | ((value & 0x00FF000000000000) >> 40) |
           ((value & 0x0000FF0000000000) >> 24) | ((value & 0x000000FF00000000) >> 8) |
           ((value & 0x00000000FF000000) << 8) | ((value & 0x0000000000FF0000) << 24) |
           ((value & 0x000000000000FF00) << 40) | ((value & 0x00000000000000FF) << 56);
}

uint32_t be2le(const uint32_t value)
{
    return ((value & 0xFF000000) >> 24) | ((value & 0x00FF0000) >> 8) | ((value & 0x0000FF00) << 8) |
           ((value & 0x000000FF) << 24);
}

// To be called from prepC.S of early boot age, only once when cold boot
// Note that here we have limited stack and heap (Totally 64KB by default), be careful!
long k_early_boot(const unsigned long hart_id, const void *dtb_addr, void **sys_stack_base)
{

    int ret = fdt_check_header(dtb_addr);
    if (ret != 0)
    {
        puts("[EBOOT] FDT header check failed\n");
        return -1;
    }

    ret = fdt_path_offset(dtb_addr, "/");
    if (ret < 0)
    {
        puts("[EBOOT] FDT root node not found\n");
        return -2;
    }
#ifdef DEBUG
    print_node(dtb_addr, 0, 0);
#endif

    // Detect system memory size
    ret = fdt_path_offset(dtb_addr, "/memory");
    if (ret < 0)
    {
        puts("[EBOOT] FDT memory node not found\n");
        return -3;
    }

    int regsize = 0;
    const void *memreg = fdt_getprop(dtb_addr, ret, "reg", &regsize);
    if (!memreg || regsize < 16)
    {
        puts("[EBOOT] FDT memory reg invalid\n");
        return -4;
    }

    unsigned long mem_start = be2le64(((const unsigned long *)memreg)[0]);
    unsigned long mem_len = be2le64(((const unsigned long *)memreg)[1]);

    printf("[EBOOT] Memory Range: base= 0x%lx, len= 0x%lx\n", mem_start, mem_len);

    // Get the kernel command line
    ret = fdt_path_offset(dtb_addr, "/chosen");
    if (ret < 0)
    {
        puts("[EBOOT] FDT chosen node not found\n");
        return -5;
    }
    const void *kcmdline = fdt_getprop(dtb_addr, ret, "bootargs", &regsize);
    if (!kcmdline || regsize < 1)
    {
        puts("[EBOOT] FDT bootargs invalid\n");
        return -6;
    }
    memcpy(kernel_args, kcmdline, regsize);
    puts("[EBOOT] Kernel command line: ");
    puts(kernel_args);

    // Parse Kerenl command line args for memory size (force override if specified in cmdline, only the first one is
    // used!)
    kernel_cmdargc = split_args((char *)kernel_args, kernel_args_array);
    unsigned long mem_len_cmdarg = 0;
    char mem_suffix = 'M';
    for (int i = 0; i < kernel_cmdargc; ++i)
    {
        if (strncmp(kernel_args_array[i], "-mem", 4) == 0)
        {
            if (i < kernel_cmdargc - 1)
            {
                int res = sscanf(kernel_args_array[i + 1], "%ld %c", &mem_len_cmdarg, &mem_suffix);
                if (res == 0)
                {
                    puts("[EBOOT] Invalid memory size specified, skipping\n");
                    mem_len_cmdarg = 0;
                    break;
                }
                if (res == 1)
                {
                    puts("[EBOOT] Memory size specified without suffix, assuming MB\n");
                    mem_len_cmdarg *= 1024 * 1024;
                    break;
                }
                if (res == 2)
                {
                    switch (mem_suffix)
                    {
                    case 'K':
                    case 'k':
                        mem_len_cmdarg *= 1024;
                        break;
                    case 'M':
                    case 'm':
                        mem_len_cmdarg *= 1024 * 1024;
                        break;
                    case 'G':
                    case 'g':
                        mem_len_cmdarg *= 1024 * 1024 * 1024;
                        break;
                    default:
                        puts("[EBOOT] Invalid memory size suffix, skipping\n");
                        mem_len_cmdarg = 0;
                        break;
                    }
                    break;
                }
            }
            else
            {
                puts("[EBOOT] Memory size not specified, skipping\n");
                mem_len_cmdarg = 0;
                break;
            }
        }
    }

    // Copy device tree to specified address
    int fdtsize = fdt_totalsize(dtb_addr);
    if (fdtsize > K_FDT_MAX_SIZE)
    {
        puts("[EBOOT] FDT too large\n");
        return -7;
    }
    memcpy(k_fdt, dtb_addr, fdtsize);

    // set the system stack base
    k_mem_size = (mem_len_cmdarg > 0) ? mem_len_cmdarg : mem_len;

    *sys_stack_base = (void *)(mem_start + k_mem_size);
    printf("[EBOOT] Set SYS_SP: 0x%lx\n", (unsigned long)*sys_stack_base);

    // init C++ exceptions
    extern void *__eh_frame_start;
    extern void __register_frame(void *); // not knowing the prototype of __register_frame, guess and OK!
    __register_frame(&__eh_frame_start);

    return 0; // Non-zero to indicate error and hang the system
}
