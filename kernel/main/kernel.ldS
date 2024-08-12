/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

OUTPUT_ARCH(riscv)
ENTRY(_entry)

_KERNEL_VADDR_BASE = 0xFFFFFFC000000000;
_DRAM_BASE = 0x80000000;
_KERNEL_OFFSET = 0x100000;


SECTIONS
{

	. = _DRAM_BASE + _KERNEL_OFFSET; /* For most case, starting from DDR by previous BL */
	
	. = ALIGN(0x1000); /* Need this to create proper sections */

	/* Beginning of the code section */
	PROVIDE(_kernel_start = .);

	.entry  (READONLY) :
	{
		*(.entry)
		*(.entry.data)
		. = ALIGN(64);
		PROVIDE(_entry_pgtbl_start = .);
		KEEP(*(.entry.pgtbl.*))
		PROVIDE(_entry_pgtbl_end = .);
	}

	. += _KERNEL_VADDR_BASE; 

	.text (READONLY): AT (_text_start - _KERNEL_VADDR_BASE)
	{
		PROVIDE(_text_start = .);
		*(.startup)
		*(.startup.*)
		*(.text)
		*(.text.*)
		*(.gcc.*)
		*(.gcc_except_table)
		*(.gcc_except_table.*)
		. = ALIGN(8);
		PROVIDE(_text_end = .);
	}
	/* . = ALIGN(0x1000); */

	.cdtors (READONLY):
	{
	  PROVIDE_HIDDEN (_init_array_start = .);
	  KEEP (*(SORT_BY_INIT_PRIORITY(.init_array.*) SORT_BY_INIT_PRIORITY(.ctors.*)))
	  KEEP (*(.init_array EXCLUDE_FILE (*crtbegin.o *crtbegin?.o *crtend.o *crtend?.o ) .ctors))
	  . = ALIGN(8);
	  PROVIDE_HIDDEN (_init_array_end = .);

	  PROVIDE_HIDDEN (_fini_array_start = .);
	  KEEP (*(SORT_BY_INIT_PRIORITY(.fini_array.*) SORT_BY_INIT_PRIORITY(.dtors.*)))
	  KEEP (*(.fini_array EXCLUDE_FILE (*crtbegin.o *crtbegin?.o *crtend.o *crtend?.o ) .dtors))
	  . = ALIGN(8);
	  PROVIDE_HIDDEN (_fini_array_end = .);
	} 
	
	/* . = ALIGN(0x1000); */

	.eh_frame (READONLY):
	{
		PROVIDE(__eh_frame_start = .);
		*(.eh_frame);
		QUAD(0); /* Force terminate the eh_frame */
	}

	/* End of the code sections */

	/* . = ALIGN(0x1000); */
	/* Beginning of the read-only data sections */

	.got : 
	{
		PROVIDE(_got_start = .);
		*(.got)
		*(.got.*)
		. = ALIGN(8);
		PROVIDE(_got_end = .);
	}

	.rodata (READONLY):
	{
		PROVIDE(_rodata_start = .);
		*(.rodata .rodata.*)

		*(.srodata.cst16)
    	*(.srodata.cst8)
    	*(.srodata.cst4)
    	*(.srodata.cst2)
    	*(.srodata .srodata.*)
		. = ALIGN(8);
		PROVIDE(_rodata_end = .);
	}

	/* . = ALIGN(0x1000); */

	PROVIDE(_tls_len = (_tbss_end - _tbss_start) + (_tdata_end - _tdata_start));

	/* . = ALIGN(0x1000); */

	.data :
	{
		PROVIDE(_data_start = .);

		*(.data)
		*(.data.*)
		*(.readmostly.data)
		*(*.data)
		. = ALIGN(8);
		PROVIDE(__global_pointer$ = . + 0x800);
		*(.sdata)
		*(.sdata.*)
		*(.readmostly.sdata)
		*(*.sdata)
		. = ALIGN(8);
		PROVIDE(_data_end = .);
	}

	/* . = ALIGN(0x1000); */

	.bss :
	{
		PROVIDE(_bss_start = .);
		*(.sbss)
		*(.sbss.*)

		*(.bss)
		*(.bss.*)
		. = ALIGN(8);
		PROVIDE(_bss_end = .);
	}

	.tdata (READONLY):
	{

		PROVIDE(_tdata_start = .);

		*(.tdata)
		*(.tdata.*)
		. = ALIGN(8);
		PROVIDE(_tdata_end = .);
	} 

	/* In TLS, do not add align 4K, otherwise addressing to tbss will need to add a 4k from $tp which cost much space */

	.tbss (READONLY): 
	{
		PROVIDE(_tbss_start = .);
		*(.tbss)
		*(.tbss.*)
		. = ALIGN(8);
		PROVIDE(_tbss_end = .);
	}

	. = ALIGN(0x1000); /* Need this to create proper sections */
	PROVIDE(_kernel_end = .);

	
	
	/* . = . + _umode_end - _umode_start; */
	
	/* Provide heap end  */
	__heap_start__ = .;
    end = __heap_start__;

	/* .other (NOLOAD) :
	{
		*(.*)
		*(.*.*)
	}  */

	/* _umode_start = 0xffff800000000000; 
	OVERLAY (_umode_start) : AT(_kernel_end) {
		
		.umode1
		{
			*(.umode1.*)
			. = ALIGN(8);
		} 

	} 
	_umode_end = .; */

	/* . = .+ _KERNEL_STACK_SIZE; */

}
