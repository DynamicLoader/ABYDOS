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

	.section .startup, "ax", %progbits
	.align 3
	.globl _start
_start: // func start(s0: hartid, s1: fdt addr)
	.option push
  	.option norelax
	lla gp, __global_pointer$
  	.option pop

	/* Zero-out BSS */
	lla	a4, _bss_start
	lla	a5, _bss_end
_bss_zero:
	REG_S	zero, (a4)
	add	a4, a4, __SIZEOF_POINTER__
	blt	a4, a5, _bss_zero

_boot:
	/* Disable and clear all interrupts */
	csrw	CSR_SIE, zero
	csrw	CSR_SIP, zero
	lla a3, _start_hang
	csrw	CSR_STVEC, a3

	/* Setup boot stack */
	# lla	a3, _kernel_end
	# li	a4, K_CONFIG_KERNEL_STACK_SIZE
	# slli a4, a4, 3
	# add	sp, a3, a4
	# lla a3, _sys_stack_base
	# REG_S	sp, 0(a3)
	li sp, K_START_STACK_ADDR

	/* Early boot in C */
	mv a0, s0
	mv a1, s1
	call k_boot

	bne a0, zero, _kernel_exit 

	call _setup_hart // setup boot hart first

	/* Boot Stage 2: boot other harts */
	mv a0, s0
	call k_boot_harts

_start_warm:

	mv a0, s0
	call  	k_pre_main
	bnez 	a0, _kernel_exit
	call	k_main
	call	k_after_main

_kernel_exit:
	mv s2, a0
	call k_before_cleanup
	mv a0, s2

	/* recover to global context */
	li sp, K_START_STACK_ADDR
	mv tp, zero

	call k_cleanup

	/* We don't expect to reach here hence just hang */
	j	_start_hang

	.globl _start_hang
_start_hang:
	wfi
	j	_start_hang

	.globl _start_hart
_start_hart:
	/* s0: hartid s1: stack base */
	REG_L a2, -16(s1)
	csrw  CSR_SATP, a2
	fence.i 
	sfence.vma zero, zero
	fence.i

	.option push
  	.option norelax
 	lla gp, __global_pointer$
  	.option pop

	mv a1, s1
	call 	_setup_hart
	mv a0, s0
	call 	k_pre_main
	bnez 	a0, _exit_hart
	call	k_main
	call 	k_after_main

_exit_hart:

	// Try to halt the hart
#define SBI_EXT_HSM				0x48534D
#define SBI_EXT_HSM_HART_STOP			0x1

	li a7, SBI_EXT_HSM 
	li a6, SBI_EXT_HSM_HART_STOP
	ecall

	// Not expected to reach here, but just in case
	j _start_hang


_setup_hart: // func setup_hart(a1: stack addr)
	/* Disable and clear all interrupts */
	csrw	CSR_SIE, zero
	csrw	CSR_SIP, zero

	/* Setup exception vectors */
	# lla	a3, _start_hang
	# csrw	CSR_STVEC, a3

	/* Setup system stack base according to hart_id as offset */
	# lla a3, _sys_stack_base
	# REG_L sp, 0(a3)
	# li  a2, K_CONFIG_STACK_SIZE
	# mul a3, a0, a2
	# sub sp, sp, a3
	mv sp, a1

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

	.section .rodata
	.align	3
	.global _entry_hart_addr
_entry_hart_addr:
	RISCV_PTR	_entry_hart	
