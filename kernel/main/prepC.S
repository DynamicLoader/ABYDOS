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
#include <sbi/riscv_elf.h>
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

	/* relocate  */
# 	lla	t0, _link_start
# 	REG_L	t0, 0(t0)
# 	/* t1 shall has the address of _fw_start */
# 	sub	t2, t1, t0
# 	lla	t0, __rel_dyn_start
# 	lla	t1, __rel_dyn_end
# 	beq	t0, t1, _relocate_done
# 2:
# 	REG_L	t5, REGBYTES(t0)	/* t5 <-- relocation info:type */
# 	li	t3, R_RISCV_RELATIVE	/* reloc type R_RISCV_RELATIVE */
# 	bne	t5, t3, 3f
# 	REG_L	t3, 0(t0)
# 	REG_L	t5, (REGBYTES * 2)(t0)	/* t5 <-- addend */
# 	add	t5, t5, t2
# 	add	t3, t3, t2
# 	REG_S	t5, 0(t3)		/* store runtime address to the GOT entry */

# 3:
# 	addi	t0, t0, (REGBYTES * 3)
# 	blt	t0, t1, 2b
# 	# j	_relocate_done
# _relocate_done:

_start_warm:
	/* Disable and clear all interrupts */
	csrw	CSR_SIE, zero
	csrw	CSR_SIP, zero

	/* Setup exception vectors */
	lla	a3, _start_hang
	csrw	CSR_STVEC, a3

	/* Setup stack */
	lla	a3, _payload_end
	li	a4, 0x2000
	add	sp, a3, a4

	/* Jump to C main */
	lla	a3, _boot_a0
	REG_L	a0, 0(a3)
	lla	a3, _boot_a1
	REG_L	a1, 0(a3)
	call	k_cstart /* start kernel */

	/* We don't expect to reach here hence just hang */
	j	_start_hang

	.section .entry, "ax", %progbits
	.align 3
	.globl _start_hang
_start_hang:
	wfi
	div t2, t2,zero
	div t2, t2,zero
	div t2, t2,zero
	j	_start_hang

	.section .data
	.align	3
_hart_lottery:
	RISCV_PTR	0
# _link_start:
# 	RISCV_PTR	_payload_start
_boot_a0:
	RISCV_PTR	0
_boot_a1:
	RISCV_PTR	0

