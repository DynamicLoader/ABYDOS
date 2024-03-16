/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */
#include <sbi/riscv_asm.h>
#include <sbi/riscv_encoding.h>

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
	bnez	a3, _start_hang

	/* Save a0 and a1 */
	lla	a3, _boot_a0
	REG_S	a0, 0(a3)
	lla	a3, _boot_a1
	REG_S	a1, 0(a3)

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
	lla	a4, _KERNEL_BOOT_STACK_SIZE /* this is fake a address provided from ld */
	add	sp, a3, a4
	lla a3, _sys_stack_base
	REG_S	sp, 0(a3)

	/* Early boot in C */
	lla	a3, _boot_a1
	REG_L	a0, 0(a3)
	call k_early_boot

	bne a0, zero, _kernel_hang 

	/* Boot stage 1: Start Kernel premain */
	lla a0, _sys_stack_base
	call k_premain_0
	bne a0, zero, _kernel_hang

_start_warm:
	/* Disable and clear all interrupts */
	csrw	CSR_SIE, zero
	csrw	CSR_SIP, zero

	/* Setup exception vectors */
	lla	a3, _kernel_hang
	csrw	CSR_STVEC, a3

	/* Setup system stack base */
	lla a3, _sys_stack_base
	REG_L sp, 0(a3)

	/* Boot stage 2: Start Main */
	lla	a3, _boot_a0
	REG_L	a0, 0(a3)
	call	k_main

	/* And the last thing: after main */
_kernel_hang:
	call k_after_main

	/* We don't expect to reach here hence just hang */
	j	_start_hang

	.section .entry, "ax", %progbits
	.align 3
	.globl _start_hang
_start_hang:
	wfi
	j	_start_hang

	.section .data
	.align	3
_hart_lottery:
	RISCV_PTR	0
# _link_start:
# 	RISCV_PTR	_payload_start
_boot_a0:  // hart id
	RISCV_PTR	0
_boot_a1: // dtb address
	RISCV_PTR	0
_sys_stack_base:
	RISCV_PTR	0
_load_start:
	RISCV_PTR   _kernel_start