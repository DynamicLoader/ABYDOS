
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
	.globl _entry // This is the entry point for the first hart
_entry: // func entry(a0: hartid, a1: fdt addr)
	/* Pick one hart to run the main boot sequence */
	lla	a3, _hart_lottery
	li	a2, 1
	amoadd.w a3, a2, (a3)
	bnez	a3, _entry_hang // in case multi core start at the same time

	/* Save a0 and a1 */
	mv s0, a0
	mv s1, a1

	lla a3, _entry_hang
	csrw	CSR_STVEC, a3

	call _setup_pgtbl

_jump_start:
	lla a0, _start_addr
	REG_L a0, 0(a0)
	jr a0

	.globl _entry_hart // This is the entry point for the other harts or resume from suspend
_entry_hart: // func entry_hart(a0: hartid, a1: stack addr)
	/* Save a0 and a1 */
	mv s0, a0
	mv s1, a1

	lla a3, _entry_hang
	csrw	CSR_STVEC, a3

	lla a5, _startup_pgtbl
	call _set_mmu
	
	// We get the jump address from the stack
	// Since the MMU is temporarily ON for the jump, we could access to the stack 
	REG_L a1, -8(a1)
	jr a1

_setup_pgtbl:

	/* Get the MVENDORID */
	li a7, 0x10
	li a6, 4
	ecall
	bnez a0, _entry_hang
	mv s2, a1

	/* Get marchid */
	li a6, 0x5
	ecall
	bnez a0, _entry_hang
	mv s3, a1

	/* Get mimpid */
	li a6, 0x6
	ecall
	bnez a0, _entry_hang
	mv s4, a1

	lla a0, _entry_pgtbl_start
	lla a1, _entry_pgtbl_end
	lla a4, _default_pgtbl

_find_pgtbl_loop:
	beq a0, a1, _load_pgtbl
	/* check magic */
	REG_L a2, 0(a0)
	li a3, 0xffffffff
	and a2, a2, a3
	li a3, 0xb00797b1
	bne a2, a3, _find_pgtbl_next
	/* check MVENDORID */
	REG_L a2, 8(a0)
	bne a2, s2, _find_pgtbl_next
	/** check MARCHID */
	REG_L a2, 16(a0)
	bne a2, s3, _find_pgtbl_next
	/* check MIMPID */
	REG_L a2, 24(a0)
	bne a2, s4, _find_pgtbl_next
	addi a4, a0, 32 
	j _load_pgtbl

_find_pgtbl_next:
	addi a0, a0, 64
	j _find_pgtbl_loop


_load_pgtbl: /* func load_pgtbl(a4: tbl) */
	lla a5, _startup_pgtbl
	addi a5, a5, 1024
	REG_L a3, 0(a4)
	REG_S a3, -1024(a5)
	REG_S a3, 1024(a5)
	REG_L a3, 8(a4)
	REG_S a3, (8-1024)(a5)
	REG_S a3, (8+1024)(a5)
	REG_L a3, 16(a4)
	REG_S a3, (16-1024)(a5)
	REG_S a3, (16+1024)(a5)
	REG_L a3, 24(a4)
	REG_S a3, (24-1024)(a5)
	REG_S a3, (24+1024)(a5)

_set_mmu: 
	/* Set the SATP */
	srli a5, a5, 12
	li a4, 8 /* SV39 */
	slli a4, a4, 60
	or a5, a4, a5
	csrw CSR_SATP, a5
	fence.i
	sfence.vma zero, zero
	fence.i

    /* if the mmu does not setup correctly the hart will hang into machine mode */

    lla t0, _mmutest
    li t1,1
    amoadd.w t2,t1,(t0)
    li t2, 0xFFFFFFC0
    slli t2, t2, 32
    or t0, t0, t2
    amoadd.w t2,t1,(t0)

	ret

_entry_hang:
	wfi
	j	_entry_hang

	.section .entry.data
	.align 3
_hart_lottery:
	RISCV_PTR	0
_start_addr:
	RISCV_PTR	_start
_mmutest:
	RISCV_PTR	0

_default_pgtbl:
	.quad 0xef
	.quad 0x100000EF
	.quad 0x200000EF
	.quad 0x300000EF

	.align 12
_startup_pgtbl:
	.fill 512, 8, 0

/* Some platform does need special bits in page table to work, 
 * so we need to provide a way to customize the page table 
 
    Each item is 64 bytes, with the following format:
    Magic       word    0xb00797b1  // BOOT gTBL
    Reserved    word    0 
    MVENDORID   quad    MVENDORID of the platform
    MARCHID     quad    MARCHID of the platform
    MIMPID      quad    MIMPID of the platform
    0G          quad    0G of the page table
    1G          quad    1G of the page table
    2G          quad    2G of the page table
    3G          quad    3G of the page table
 */

	.section .entry.pgtbl.c906
	.align 6
_pgtbl_c906:
	.word 0xb00797b1
	.word 0 /* reserved */
	.quad  0x5b7 /* MVENDORID */
	.quad 0x0 /* MARCHID */
	.quad 0x0 /* MIMPID */
	.quad 0xef
	.quad 0x100000EF
	.quad 0x200000EF | (0x180 << 54)
	.quad 0x300000EF | (0x180 << 54)