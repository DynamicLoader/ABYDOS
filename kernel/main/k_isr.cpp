
#include <stdio.h>

#include "k_defs.h"
#include "k_main.h"
#include "k_sbif.hpp"

#define _K_ISR_SP_OFFSET -8
#define _K_ISR_TP_OFFSET -16
#define _K_ISR_GP_OFFSET -24
#define SAVE_SPACE (2 + 1 + 3 + 8 + 4)
#if __riscv_xlen == 64
#define __REG_SEL(a, b) #a
#define REG_SIZE 8

#define RISCV_PTR .dword
#elif __riscv_xlen == 32
#define __REG_SEL(a, b) #b
#define REG_SIZE 4
#define RISCV_PTR .word
#else
#error "Unexpected __riscv_xlen"
#endif

#define REG_L __REG_SEL(ld, lw)
#define REG_S __REG_SEL(sd, sw)

#define _STR(x) #x
#define _VSTR(x) _STR(x)

// clang-format off

#define K_ISR_SWITCH_SP() \
    asm volatile( \
        "csrrw sp, sscratch, sp \n" \
        /* We'd judge that: if sscratch == 0, then from S-mode; otherwise from U-mode */ \
        "bnez sp, 1f \n" \
        "csrrw sp, sscratch, sp \n"  \
        "1: \n" \
    )

#define K_ISR_SAVE_CONTEXT() \
    asm volatile( \
        "add sp, sp, -" _VSTR(SAVE_SPACE * REG_SIZE) " \n" \
        REG_S " ra, " _VSTR(0 * REG_SIZE) "(sp) \n" \
        REG_S " gp, " _VSTR(1 * REG_SIZE) "(sp) \n" \
        REG_S " tp, " _VSTR(2 * REG_SIZE) "(sp) \n" \
        REG_S " t0, " _VSTR(3 * REG_SIZE) "(sp) \n" \
        REG_S " t1, " _VSTR(4 * REG_SIZE) "(sp) \n" \
        REG_S " t2, " _VSTR(5 * REG_SIZE) "(sp) \n" \
        REG_S " t3, " _VSTR(6 * REG_SIZE) "(sp) \n" \
        REG_S " t4, " _VSTR(7 * REG_SIZE) "(sp) \n" \
        REG_S " t5, " _VSTR(8 * REG_SIZE) "(sp) \n" \
        REG_S " t6, " _VSTR(9 * REG_SIZE) "(sp) \n" \
        REG_S " a0, " _VSTR(10 * REG_SIZE) "(sp) \n" \
        REG_S " a1, " _VSTR(11 * REG_SIZE) "(sp) \n" \
        REG_S " a2, " _VSTR(12 * REG_SIZE) "(sp) \n" \
        REG_S " a3, " _VSTR(13 * REG_SIZE) "(sp) \n" \
        REG_S " a4, " _VSTR(14 * REG_SIZE) "(sp) \n" \
        REG_S " a5, " _VSTR(15 * REG_SIZE) "(sp) \n" \
        REG_S " a6, " _VSTR(16 * REG_SIZE) "(sp) \n" \
        REG_S " a7, " _VSTR(17 * REG_SIZE) "(sp) \n" \
        "mv tp, sp \n" \
        "li gp, " _VSTR(K_CONFIG_KERNEL_STACK_SIZE - 1) "\n" \
        "and gp, gp, sp \n" \
        "srai tp, tp, " _VSTR(K_CONFIG_STACK_SIZE_N) "\n" \
        "beqz gp, 1f \n" /* No stack space used, do not add 1 to recover */ \
        "addi tp, tp, 1 \n" \
        "1:" \
        "slli tp, tp, " _VSTR(K_CONFIG_STACK_SIZE_N) "\n" \
        "lla gp, _tls_len \n" \
        "sub tp, tp, gp \n" \
        ".option push \n" \
        ".option norelax \n" \
        "lla gp, __global_pointer$ \n" \
        ".option pop \n" \
    )

#define K_ISR_RESTORE_CONTEXT(ret) \
    asm volatile( \
        REG_L " ra, " _VSTR(0 * REG_SIZE) "(sp) \n" \
        REG_L " gp, " _VSTR(1 * REG_SIZE) "(sp) \n" \
        REG_L " tp, " _VSTR(2 * REG_SIZE) "(sp) \n" \
        REG_L " t0, " _VSTR(3 * REG_SIZE) "(sp) \n" \
        REG_L " t1, " _VSTR(4 * REG_SIZE) "(sp) \n" \
        REG_L " t2, " _VSTR(5 * REG_SIZE) "(sp) \n" \
        REG_L " t3, " _VSTR(6 * REG_SIZE) "(sp) \n" \
        REG_L " t4, " _VSTR(7 * REG_SIZE) "(sp) \n" \
        REG_L " t5, " _VSTR(8 * REG_SIZE) "(sp) \n" \
        REG_L " t6, " _VSTR(9 * REG_SIZE) "(sp) \n" \
        REG_L " a0, " _VSTR(10 * REG_SIZE) "(sp) \n" \
        REG_L " a1, " _VSTR(11 * REG_SIZE) "(sp) \n" \
        REG_L " a2, " _VSTR(12 * REG_SIZE) "(sp) \n" \
        REG_L " a3, " _VSTR(13 * REG_SIZE) "(sp) \n" \
        REG_L " a4, " _VSTR(14 * REG_SIZE) "(sp) \n" \
        REG_L " a5, " _VSTR(15 * REG_SIZE) "(sp) \n" \
        REG_L " a6, " _VSTR(16 * REG_SIZE) "(sp) \n" \
        REG_L " a7, " _VSTR(17 * REG_SIZE) "(sp) \n" \
        "add sp, sp, " _VSTR(SAVE_SPACE * REG_SIZE) "\n"  \
    )

// clang-format on

/* #define K_ISR_SAVE_AND_CALL(func) \ \
    K_ISR_SAVE_CONTEXT();                                                                                              \
    asm volatile("call " #func);

#define K_ISR_RESTORE_AND_RET(ret)                                                                                     \
    K_ISR_RESTORE_CONTEXT();                                                                                           \
    asm volatile(#ret "\n"); */

#define K_ISR_ENTRY_IMPL(name, func)                                                                                   \
    K_ISR_ENTRY void name()                                                                                            \
    {                                                                                                                  \
        K_ISR_SWITCH_SP();                                                                                             \
        K_ISR_SAVE_CONTEXT();                                                                                          \
        asm volatile("call " #func "\n");                                                                              \
        K_ISR_RESTORE_CONTEXT();                                                                                       \
        K_ISR_SWITCH_SP();                                                                                             \
        asm volatile("sret");                                                                                          \
    }

K_ISR void isr_softirq()
{
    printf("Softirq interrupt for hart %i\n",hartid);
}

K_ISR void isr_timer()
{
    auto time = csr_read(CSR_TIME);
    printf("Timer interrupt for hart %i\n",hartid);
    printf("Current Time: %ld\n", time);
    auto rc = SBIF::Timer::setTimer(time + 10000000);
    if(rc)
        printf("Cannot reset timer: %ld\n", rc);
}

K_ISR void isr_extirq()
{
    printf("External interrupt for hart %i\n",hartid);
}

K_ISR_ENTRY_IMPL(k_softirq_entry, isr_softirq)
K_ISR_ENTRY_IMPL(k_timer_entry, isr_timer)
K_ISR_ENTRY_IMPL(k_extirq_entry, isr_extirq)
