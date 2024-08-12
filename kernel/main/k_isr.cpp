
#include <stdio.h>

#include "k_defs.h"
#include "k_main.h"
#include "k_umode.h"
#include "syscall.h"
#include "k_sbif.hpp"

#define SAVE_SPACE 32 // the max space used for saving context
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

#define _K_ISR_SAVE_CONTEXT_NORMAL \
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

#define _K_ISR_SAVE_CONTEXT_ADDITIONAL \
        REG_S " s0, " _VSTR(18 * REG_SIZE) "(sp) \n" \
        REG_S " s1, " _VSTR(19 * REG_SIZE) "(sp) \n" \
        REG_S " s2, " _VSTR(20 * REG_SIZE) "(sp) \n" \
        REG_S " s3, " _VSTR(21 * REG_SIZE) "(sp) \n" \
        REG_S " s4, " _VSTR(22 * REG_SIZE) "(sp) \n" \
        REG_S " s5, " _VSTR(23 * REG_SIZE) "(sp) \n" \
        REG_S " s6, " _VSTR(24 * REG_SIZE) "(sp) \n" \
        REG_S " s7, " _VSTR(25 * REG_SIZE) "(sp) \n" \
        REG_S " s8, " _VSTR(26 * REG_SIZE) "(sp) \n" \
        REG_S " s9, " _VSTR(27 * REG_SIZE) "(sp) \n" \
        REG_S " s10, " _VSTR(28 * REG_SIZE) "(sp) \n" \
        REG_S " s11, " _VSTR(29 * REG_SIZE) "(sp) \n" \
        
    
#define _K_ISR_SAVE_RECALC_PTRS \
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

#define K_ISR_SAVE_CONTEXT_NORMAL() \
    asm volatile( \
        "add sp, sp, -" _VSTR(SAVE_SPACE * REG_SIZE) " \n" \
        REG_S " ra, " _VSTR(0 * REG_SIZE) "(sp) \n" \
        REG_S " gp, " _VSTR(1 * REG_SIZE) "(sp) \n" \
        REG_S " tp, " _VSTR(2 * REG_SIZE) "(sp) \n" \
        _K_ISR_SAVE_CONTEXT_NORMAL \
        _K_ISR_SAVE_RECALC_PTRS \
    )

#define K_ISR_SAVE_CONTEXT_ADDITIONAL() \
    asm volatile( \
        "add sp, sp, -" _VSTR(SAVE_SPACE * REG_SIZE) " \n" \
        REG_S " ra, " _VSTR(0 * REG_SIZE) "(sp) \n" \
        REG_S " gp, " _VSTR(1 * REG_SIZE) "(sp) \n" \
        REG_S " tp, " _VSTR(2 * REG_SIZE) "(sp) \n" \
        _K_ISR_SAVE_CONTEXT_NORMAL \
        _K_ISR_SAVE_CONTEXT_ADDITIONAL \
        _K_ISR_SAVE_RECALC_PTRS \
    )

#define K_ISR_RESTORE_CONTEXT_NORMAL() \
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
        "add sp, sp, " _VSTR(SAVE_SPACE * REG_SIZE) "\n" /* Stack Balance */ \
    )

#define K_ISR_RESTORE_CONTEXT_ADDITIONAL() \
    asm volatile( \
        REG_L " tp, " _VSTR(31 * REG_SIZE) "(sp) \n"  /* PC */ \
        "csrw sepc, tp \n" \
        REG_L " tp, " _VSTR(30 * REG_SIZE) "(sp) \n" /* SP */\
        "csrw sscratch, tp \n" \
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
        REG_L " s0, " _VSTR(18 * REG_SIZE) "(sp) \n" \
        REG_L " s1, " _VSTR(19 * REG_SIZE) "(sp) \n" \
        REG_L " s2, " _VSTR(20 * REG_SIZE) "(sp) \n" \
        REG_L " s3, " _VSTR(21 * REG_SIZE) "(sp) \n" \
        REG_L " s4, " _VSTR(22 * REG_SIZE) "(sp) \n" \
        REG_L " s5, " _VSTR(23 * REG_SIZE) "(sp) \n" \
        REG_L " s6, " _VSTR(24 * REG_SIZE) "(sp) \n" \
        REG_L " s7, " _VSTR(25 * REG_SIZE) "(sp) \n" \
        REG_L " s8, " _VSTR(26 * REG_SIZE) "(sp) \n" \
        REG_L " s9, " _VSTR(27 * REG_SIZE) "(sp) \n" \
        REG_L " s10, " _VSTR(28 * REG_SIZE) "(sp) \n" \
        REG_L " s11, " _VSTR(29 * REG_SIZE) "(sp) \n" \
        "add sp, sp, " _VSTR(SAVE_SPACE * REG_SIZE) "\n" /* Stack Balance */ \
    )

// clang-format on

#define K_ISR_ENTRY_IMPL(mode, name, func)                                                                             \
    K_ISR_ENTRY void name()                                                                                            \
    {                                                                                                                  \
        K_ISR_SWITCH_SP();                                                                                             \
        K_ISR_SAVE_CONTEXT_##mode();                                                                                   \
        asm volatile("mv a0, sp \n"                                                                                    \
                     "call " #func "\n");                                                                              \
        K_ISR_RESTORE_CONTEXT_##mode();                                                                                \
        K_ISR_SWITCH_SP();                                                                                             \
        asm volatile("sret");                                                                                          \
    }

K_ISR void k_isr_softirq(saved_context_t *ctx)
{
    printf("Software interrupt for hart %i\n", hartid);
    extern thread_local bool k_halt;
    k_halt = true;
    csr_clear(CSR_SIP, SIP_SSIP);
    if (k_local_resume)
    {
        csr_write(CSR_SEPC, k_local_resume);
        csr_set(CSR_SSTATUS, SSTATUS_SPP);
        csr_clear(CSR_SIE, SIP_SSIP);
        csr_write(CSR_SSCRATCH, 0);
        printf("Prepare to resume from U-mode\n");
    }
}

K_ISR void k_isr_timer(umode_basic_ctx_t *uctx)
{
    auto time = csr_read(CSR_TIME);
    printf("Timer interrupt for hart %i\n", hartid);
    printf("Current Time: %ld\n", time);
    if (time > 10 * k_cpuclock)
    {
        SBIF::IPI::sendIPI(-1, 0);
        SBIF::Timer::clearTimer();
        return;
    }
    auto rc = SBIF::Timer::setTimer(time + k_cpuclock);
    if (rc)
        printf("Cannot reset timer: %ld\n", rc);
}

K_ISR void k_isr_extirq(saved_context_t *ctx)
{
    printf("External interrupt for hart %i\n", hartid);
}

K_ISR void k_esr_ecall(umode_basic_ctx_t *uctx)
{
    printf("ECall from U-mode at 0x%lx\n", csr_read(CSR_SEPC));
    auto ctx = (saved_context_t *)uctx;
    if (ctx->a0 == SYSCALL_PRINTF)
    {
        printf("[UMODE] ");
        printf((const char *)(ctx->a1), ctx->a2, ctx->a3, ctx->a4); // only for test, not safe at all!
    }
    uctx->pc += 4; // ECall instruction takes 4 bytes
}

#define _DUMP_CTX(ctx)                                                                                                 \
    do                                                                                                                 \
    {                                                                                                                  \
        printf("a0 = %lx\n", ctx->a0);                                                                                 \
        printf("a1 = %lx\n", ctx->a1);                                                                                 \
        printf("a2 = %lx\n", ctx->a2);                                                                                 \
        printf("a3 = %lx\n", ctx->a3);                                                                                 \
        printf("a4 = %lx\n", ctx->a4);                                                                                 \
        printf("a5 = %lx\n", ctx->a5);                                                                                 \
        printf("a6 = %lx\n", ctx->a6);                                                                                 \
        printf("a7 = %lx\n", ctx->a7);                                                                                 \
        printf("t0 = %lx\n", ctx->t0);                                                                                 \
        printf("t1 = %lx\n", ctx->t1);                                                                                 \
        printf("t2 = %lx\n", ctx->t2);                                                                                 \
        printf("t3 = %lx\n", ctx->t3);                                                                                 \
        printf("t4 = %lx\n", ctx->t4);                                                                                 \
        printf("t5 = %lx\n", ctx->t5);                                                                                 \
        printf("t6 = %lx\n", ctx->t6);                                                                                 \
        printf("ra = %lx\n", ctx->ra);                                                                                 \
        printf("gp = %lx\n", ctx->gp);                                                                                 \
        printf("tp = %lx\n", ctx->tp);                                                                                 \
        printf("sp = %lx\n",                                                                                           \
               csr_read(CSR_SSCRATCH) == 0 ? (unsigned long)ctx - sizeof(saved_context_t) : csr_read(CSR_SSCRATCH));   \
    } while (0)

K_ISR void k_esr_break(saved_context_t *ctx)
{
    bool volatile conti = false;
    printf("=== Breakpoint at 0x%lx ===\n", csr_read(CSR_SEPC));
    _DUMP_CTX(ctx);
    printf("=== Set local variable 'conti' to true to continue ===\n");
    while (!conti)
        ;
    csr_write(CSR_SEPC, csr_read(CSR_SEPC) + 2);
}

K_ISR void k_other_exception(saved_context_t *ctx)
{
    printf("=== Unhandled exception (0x%lx) at 0x%lx ===\n", csr_read(CSR_SCAUSE), csr_read(CSR_SEPC));
    _DUMP_CTX(ctx);
    printf("STVAL = %lx\n", csr_read(CSR_STVAL));
    printf("System Hang!\n");
    while (1)
        ;
}

// clang-format off
K_ISR_ENTRY void k_exception_entry(){
    #define _K_EXC_JUMP_HELPER(func) \
        ".align 4 \n"  \
        "call 4f \n" \
        "mv a0, sp \n" \
        "call " #func " \n" \
        "j 10f \n"

    // used for fully saved context
    #define _K_EXC_JUMP_HELPER2(func) \
        ".align 4 \n"  \
        "call 5f \n" \
        "mv a0, sp \n" \
        "call " #func " \n" \
        "j 11f \n"

    #define _K_EXC_NO_HANDLER() \
        ".align 4 \n" \
        "j 3f \n"

    K_ISR_SWITCH_SP();
    asm volatile(
        // We only save necessary registers here (gp and tp are not following the ABI, as they are overwritten later)
        REG_S " ra, -" _VSTR((SAVE_SPACE - 0) * REG_SIZE) "(sp) \n" 
        REG_S " gp, -" _VSTR((SAVE_SPACE - 1) * REG_SIZE) "(sp) \n" 
        REG_S " tp, -" _VSTR((SAVE_SPACE - 2) * REG_SIZE) "(sp) \n" 

        "csrr gp, scause \n"
        "bltz gp, 3f \n" // Highest bit is 1, which means it's an interrupt
        "li tp, 16 \n"
        "bge gp, tp, 2f \n" // Exception code >= 16, not implemented!
        // Calc jump address (in tp)
        "lla tp, 1f \n"
        "slli gp, gp, 4 \n" // 16 bytes per entry
        "add tp, tp, gp \n" 
        "jr tp \n"
    );
    // Here we construct a jump table
    asm volatile(
        ".align 4 \n"
        "1: \n"
        _K_EXC_JUMP_HELPER(k_other_exception) // 0
        _K_EXC_JUMP_HELPER(k_other_exception) // 1
        _K_EXC_JUMP_HELPER(k_other_exception) // 2
        _K_EXC_JUMP_HELPER(k_esr_break) // 3 - EBreak
        _K_EXC_JUMP_HELPER(k_other_exception) // 4
        _K_EXC_JUMP_HELPER(k_other_exception) // 5
        _K_EXC_JUMP_HELPER(k_other_exception) // 6
        _K_EXC_JUMP_HELPER(k_other_exception) // 7
        _K_EXC_JUMP_HELPER2(k_esr_ecall) // 8 - ecall on U-mode
        _K_EXC_JUMP_HELPER(k_other_exception) // 9
        _K_EXC_JUMP_HELPER(k_other_exception) // 10
        _K_EXC_JUMP_HELPER(k_other_exception) // 11
        _K_EXC_JUMP_HELPER(k_other_exception) // 12
        _K_EXC_JUMP_HELPER(k_other_exception) // 13
        _K_EXC_JUMP_HELPER(k_other_exception) // 14
        _K_EXC_JUMP_HELPER(k_other_exception) // 15
    );


    // No handler, restore and hang...
    asm volatile(
        "2: \n"
        "3: \n"
        REG_L " ra, -" _VSTR((SAVE_SPACE - 0) * REG_SIZE) "(sp) \n" 
        REG_L " gp, -" _VSTR((SAVE_SPACE - 1) * REG_SIZE) "(sp) \n" 
        REG_L " tp, -" _VSTR((SAVE_SPACE - 2) * REG_SIZE) "(sp) \n" 
    );
    K_ISR_SWITCH_SP();
    asm volatile("j _start_hang \n");

    // Internal function to save context
    asm volatile(
        "4: \n"
        "add sp, sp, -" _VSTR(SAVE_SPACE * REG_SIZE) " \n" 
        /* ra, tp , gp already saved, skipped */
        _K_ISR_SAVE_CONTEXT_NORMAL
        _K_ISR_SAVE_RECALC_PTRS
        "ret \n" 
    );

    asm volatile(
        "5: \n"
        "add sp, sp, -" _VSTR(SAVE_SPACE * REG_SIZE) " \n" 
        /* ra, tp , gp already saved, skipped */
        _K_ISR_SAVE_CONTEXT_NORMAL
        _K_ISR_SAVE_CONTEXT_ADDITIONAL
        REG_S " sp, " _VSTR(30 * REG_SIZE) "(sp) \n"
        "csrr tp, sepc \n"
        REG_S " tp, " _VSTR(31 * REG_SIZE) "(sp) \n" // PC
        _K_ISR_SAVE_RECALC_PTRS
        "ret \n" 
    );

    // Internal routine to restore context
    asm volatile("10: \n");
    K_ISR_RESTORE_CONTEXT_NORMAL();
    K_ISR_SWITCH_SP();
    asm volatile("sret \n");

    asm volatile("11: \n");
    K_ISR_RESTORE_CONTEXT_ADDITIONAL();
    K_ISR_SWITCH_SP();
    asm volatile("sret \n");

    #undef _K_EXC_JUMP_HELPER
    #undef _K_EXC_NO_HANDLER
}
// clang-format on

K_ISR_ENTRY_IMPL(NORMAL, k_softirq_entry, k_isr_softirq)
K_ISR_ENTRY_IMPL(ADDITIONAL, k_timer_entry, k_isr_timer)
K_ISR_ENTRY_IMPL(NORMAL, k_extirq_entry, k_isr_extirq)

#define _EXCTABLE_JUMP_HELPER(x) ".align 2 \n j " #x " \n"
#define _EXCTABLE_JUMP_HELPER2(x) _EXCTABLE_JUMP_HELPER(x) _EXCTABLE_JUMP_HELPER(x)
#define _EXCTABLE_JUMP_HELPER4(x) _EXCTABLE_JUMP_HELPER2(x) _EXCTABLE_JUMP_HELPER2(x)
#define _EXCTABLE_JUMP_HELPER8(x) _EXCTABLE_JUMP_HELPER4(x) _EXCTABLE_JUMP_HELPER4(x)

K_ISR_ENTRY __attribute__((aligned(4))) void _exctable()
{
    asm volatile(_EXCTABLE_JUMP_HELPER(k_exception_entry) // 0x0 General exception
                 _EXCTABLE_JUMP_HELPER(k_softirq_entry)   // 0x1 Suprvior software interrupt
                 _EXCTABLE_JUMP_HELPER(_start_hang) _EXCTABLE_JUMP_HELPER2(_start_hang) // 2 - 4 Reserved

                 _EXCTABLE_JUMP_HELPER(k_timer_entry)  // 0x5 Supervisor timer interrupt
                 _EXCTABLE_JUMP_HELPER(k_extirq_entry) // 0x6 Supervisor external interrupt
                 _EXCTABLE_JUMP_HELPER(_start_hang) _EXCTABLE_JUMP_HELPER8(_start_hang) // 0x7 - 0x1F Reserved
    );
}
