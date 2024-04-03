/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#include <sbi/riscv_encoding.h>
#include "k_defs.h" 

#define __ASM_STR(x)	x

#if __riscv_xlen == 64
#define __REG_SEL(a, b)		__ASM_STR(a)
#define RISCV_PTR		.dword
#elif __riscv_xlen == 32
#define __REG_SEL(a, b)		__ASM_STR(b)
#define RISCV_PTR		.word
#else
#error "Unexpected __riscv_xlen"
#endif

#define REG_L		__REG_SEL(ld, lw)
#define REG_S		__REG_SEL(sd, sw)

	.section .entry, "ax", %progbits
	.align 3
	.globl _start
_start:
	/* Pick one hart to run the main boot sequence */
	lla	a3, _hart_lottery
	li	a2, 1
	amoadd.w a3, a2, (a3)
	bnez	a3, _start_hang // in case multi core start at the same time

	.option push
  	.option norelax
	lla gp, __global_pointer$
  	.option pop

	/* Save a0 and a1 */
	# lla	a3, _boot_a0
	# REG_S	a0, 0(a3)
	# lla	a3, _boot_a1
	# REG_S	a1, 0(a3)
	mv s0, a0
	mv s1, a1

	/* Zero-out BSS */
	lla	a4, _bss_start
	lla	a5, _bss_end
_bss_zero:
	REG_S	zero, (a4)
	add	a4, a4, __SIZEOF_POINTER__
	blt	a4, a5, _bss_zero

_prep_boot:
	/* Disable and clear all interrupts */
	csrw	CSR_SIE, zero
	csrw	CSR_SIP, zero

	/* Setup boot stack */
	lla	a3, _kernel_end
	li	a4, K_CONFIG_KERNEL_STACK_SIZE /* this is fake a address provided from ld */
	add a4, a4, a4 /* double the size */
	add a4, a4, a4 /* double the size */
	add a4, a4, a4 /* double the size */
	add	sp, a3, a4
	lla a3, _sys_stack_base
	REG_S	sp, 0(a3)

	/* Early boot in C */
	# lla	a3, _boot_a1
	# REG_L	a0, 0(a3)
	mv a0, s1
	call k_early_boot

	bne a0, zero, _kernel_exit 

	/* Boot stage 1: Start Kernel premain */
	lla a0, _sys_stack_base
	call k_boot
	bne a0, zero, _kernel_exit

	/* save system SATP to be used by other hart */
	lla 	a3, _sys_satp
	csrr 	a2, CSR_SATP
	REG_S 	a2, 0(a3)

	/* Boot Stage 2: setup perips */
	call k_boot_perip
	bne a0, zero, _kernel_exit

	mv a0, s0
	call _setup_hart // setup boot hart first

	/* Boot Stage 3: boot other harts */
	call k_boot_harts

_start_warm:

	mv a0, s0
	call  	k_premain

	mv 	a0, s0
	call	k_main

	/* And the last thing: after main */

	mv 	a1, a0
	mv 	a0, s0
	call k_after_main

_kernel_exit:
	/* recover to global context */
	lla a3, _sys_stack_base
	REG_L sp, 0(a3)
	mv tp, zero

	call k_clearup

	/* We don't expect to reach here hence just hang */
	j	_start_hang

	.section .entry, "ax", %progbits
	.align 3
	.globl _start_hang
_start_hang:
	wfi
	j	_start_hang

	.section .entry, "ax", %progbits
	.align 3
	.globl _exctable
_exctable:
	// 0x0 General exception
	j k_exception_entry
	.align 2
	// 0x1 Suprvior software interrupt
	j k_softirq_entry
	.align 2
	// 2 - 4 Reserved
	j _start_hang
	.align 2
	j _start_hang
	.align 2
	j _start_hang
	.align 2
	// 0x5 Supervisor timer interrupt
	j k_timer_entry
	.align 2
	// 0x6 Supervisor external interrupt
	j k_extirq_entry
	.align 2
	// 0x7 - 0x1F Reserved
	j _start_hang
	.align 2
	j _start_hang
	.align 2
	j _start_hang
	.align 2
	j _start_hang
	.align 2
	j _start_hang
	.align 2
	j _start_hang
	.align 2
	j _start_hang
	.align 2
	j _start_hang
	.align 2
	j _start_hang
	.align 2
	// End of exception table

	.section .entry, "ax", %progbits
	.align 3
	.globl _start_hart
_start_hart:
	/* a0: hartid a1: reserved */
	lla   a2, _sys_satp
	REG_L a2, 0(a2)
	csrw  CSR_SATP, a2
	sfence.vma zero, zero

	.option push
  	.option norelax
 	lla gp, __global_pointer$
  	.option pop

	/* Save a0 and a1 in s0 and s1 */
	mv s0, a0
	mv s1, a1

	call 	_setup_hart

	call 	k_premain

	mv a0, s0
	call	k_main

	# sub a0, zero, s0
	mv a1, a0
	li a0, -1
	call 	k_after_main

	j _start_hang


_setup_hart:
	/* Disable and clear all interrupts */
	csrw	CSR_SIE, zero
	csrw	CSR_SIP, zero

	/* Setup exception vectors */
	# lla	a3, _start_hang
	# csrw	CSR_STVEC, a3

	/* Setup system stack base according to hart_id as offset */
	lla a3, _sys_stack_base
	REG_L sp, 0(a3)
	li  a2, K_CONFIG_STACK_SIZE
	mul a3, a0, a2
	sub sp, sp, a3

_process_tbss:
	/* Setup tbss */
	lla a4, _tbss_start
	lla a5, _tbss_end
	beq a4, a5, _process_tdata

	sub a2, a5, a4
	sub sp, sp, a2
	mv  tp, sp
	mv	a3, tp

	/* Zero-out tbss */
_tbss_zero:
	REG_S	zero, (a3)
	add a3, a3, __SIZEOF_POINTER__
	add	a4, a4, __SIZEOF_POINTER__
	blt	a4, a5, _tbss_zero

_process_tdata:
	/* Setup tdata */
	lla a3, _tdata_start
	lla a2, _tdata_end
	beq a3, a2, _fini_setup_hart

	sub a2, a2, a3
	sub sp, sp, a2
	mv tp, sp

	/* Copy tdata */
	lla a4, _tdata_end
	mv a5, tp
_tdata_copy:
	REG_L a2, (a3)
	REG_S a2, (a5)
	add a3, a3, __SIZEOF_POINTER__
	add a5, a5, __SIZEOF_POINTER__
	blt a3, a4, _tdata_copy
_fini_setup_hart:
	// set ssratch to zero, so that we know the interrupt is from S-mode
	csrw CSR_SSCRATCH, zero
	ret

	.section .data
	.align	3
	.global _sys_stack_base, _mmutest
_hart_lottery:
	RISCV_PTR	0
# _boot_a0:  // hart id
# 	RISCV_PTR	0
# _boot_a1: // dtb address
# 	RISCV_PTR	0
_sys_stack_base:
	RISCV_PTR	0
_sys_satp:
	RISCV_PTR	0
_mmutest:
	RISCV_PTR	0
