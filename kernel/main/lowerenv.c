/**
 * @file k_lowerenv.c
 * @author DynamicLoader (admin@dyldr.top)
 * @brief Lower environment for kernel in C
 * @version 0.1
 * @date 2024-03-08
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include <string.h>
#include <stdio.h>

#include "k_main.h"
#include "sbif.h"

const char default_args[] = "--test -tty serial";

struct sbiret sbi_ecall(int ext, int fid, unsigned long arg0,
                        unsigned long arg1, unsigned long arg2,
                        unsigned long arg3, unsigned long arg4,
                        unsigned long arg5) {
  struct sbiret ret;

  register unsigned long a0 asm("a0") = (unsigned long)(arg0);
  register unsigned long a1 asm("a1") = (unsigned long)(arg1);
  register unsigned long a2 asm("a2") = (unsigned long)(arg2);
  register unsigned long a3 asm("a3") = (unsigned long)(arg3);
  register unsigned long a4 asm("a4") = (unsigned long)(arg4);
  register unsigned long a5 asm("a5") = (unsigned long)(arg5);
  register unsigned long a6 asm("a6") = (unsigned long)(fid);
  register unsigned long a7 asm("a7") = (unsigned long)(ext);
  asm volatile("ecall"
               : "+r"(a0), "+r"(a1)
               : "r"(a2), "r"(a3), "r"(a4), "r"(a5), "r"(a6), "r"(a7)
               : "memory");
  ret.error = a0;
  ret.value = a1;

  return ret;
}

static void sbi_console_puts(const char *str) {
  sbi_ecall(SBI_EXT_DBCN, SBI_EXT_DBCN_CONSOLE_WRITE, strlen(str),
            (unsigned long)str, 0, 0, 0, 0);
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

void k_before_main(unsigned long *pa0, unsigned long *pa1)
{
    sbi_console_puts("\n===== Entered Test Kernel =====\n");

    char buf[64];
    sprintf(buf, "a0: 0x%lx \t a1: 0x%lx\n",*pa0,*pa1);
    sbi_console_puts(buf);

    if (*pa0 == 0)
        *pa0 = (unsigned long)default_args;

    printf("test");
}

void k_after_main(int main_ret)
{
    sbi_console_puts("\n===== Test Kernel Stopped =====\n");
}

void k_cstart(unsigned long a0, unsigned long a1) // To be called from prepC.S
{
    k_before_main(&a0, &a1);

    const char *cmdargs[128] = {0};

    int cmdargc = split_args((char *)a0, cmdargs);
    int ret = k_main(cmdargc, cmdargs);

    k_after_main(ret);
    // After exit from here, infini loop in asm...
}
