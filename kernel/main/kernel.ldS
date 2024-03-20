/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

OUTPUT_ARCH(riscv)
ENTRY(_start)

_DRAM_BASE = 0x80000000;
_KERNEL_OFFSET = 0x200000;
/* _DRAM_SIZE = 64 * 1024 * 1024; */

/* Boot stack size */
_KERNEL_BOOT_STACK_SIZE = 0x100000;
_KERNEL_HART_LOCAL_DATA_SIZE = 1024;

SECTIONS
{

	. = _DRAM_BASE + _KERNEL_OFFSET; /* For most case, starting from DDR by previous BL */

	PROVIDE(_kernel_start = .);

	. = ALIGN(0x1000); /* Need this to create proper sections */

	/* Beginning of the code section */

	.init :
	{
		_sinit = .;
		. = ALIGN(8);
		KEEP(*(SORT_NONE(.init)))
		. = ALIGN(8);
		_einit = .;
	}

	. = ALIGN(0x1000);

	.text :
	{
		PROVIDE(_text_start = .);
		*(.entry)
		*(.text)
		*(.text.*)
		*(.gcc.*)
		*(.gcc_except_table.*)
		. = ALIGN(8);
		PROVIDE(_text_end = .);
	}

	. = ALIGN(0x1000);

	.init_array :
	{
	  PROVIDE_HIDDEN (_init_array_start = .);
	  KEEP (*(SORT_BY_INIT_PRIORITY(.init_array.*) SORT_BY_INIT_PRIORITY(.ctors.*)))
	  KEEP (*(.init_array EXCLUDE_FILE (*crtbegin.o *crtbegin?.o *crtend.o *crtend?.o ) .ctors))
	  . = ALIGN(8);
	  PROVIDE_HIDDEN (_init_array_end = .);
	} 
	
	. = ALIGN(0x1000);

	.fini_array :
	{
	  PROVIDE_HIDDEN (_fini_array_start = .);
	  KEEP (*(SORT_BY_INIT_PRIORITY(.fini_array.*) SORT_BY_INIT_PRIORITY(.dtors.*)))
	  KEEP (*(.fini_array EXCLUDE_FILE (*crtbegin.o *crtbegin?.o *crtend.o *crtend?.o ) .dtors))
	  . = ALIGN(8);
	  PROVIDE_HIDDEN (_fini_array_end = .);
	} 

	. = ALIGN(0x1000);

	.eh_frame :
	{
		PROVIDE(__eh_frame_start = .);
		*(.eh_frame);
	}

	/* End of the code sections */

	. = ALIGN(0x1000);
	/* Beginning of the read-only data sections */

	.rodata :
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

	. = ALIGN(0x1000);

	.tdata :
	{

		PROVIDE(_tdata_start = .);

		*(.tdata .tdata.*)
		. = ALIGN(8);
		PROVIDE(_tdata_end = .);
	} 

	/* End of the read-only data sections */

	. = ALIGN(0x1000); /* Ensure next section is page aligned */

	.tbss : {
		PROVIDE(_tbss_start = .);
		*(.tbss .tbss.*)
		. = ALIGN(8);
		PROVIDE(_tbss_end = .);
	}

	. = ALIGN(0x1000);
	/* Beginning of the read-write data sections */

	.data :
	{
		PROVIDE(_data_start = .);

		*(.data)
		*(.data.*)
		*(.readmostly.data)
		*(*.data)

		*(.sdata)
		*(.sdata.*)
		*(.readmostly.sdata)
		*(*.sdata)
		. = ALIGN(8);
		PROVIDE(_data_end = .);
	}

	. = ALIGN(0x1000); /* Ensure next section is page aligned */

	/* .sdata :
	{
		PROVIDE(_sdata_start = .);

		*(.sdata)
		*(.sdata.*)
		*(.readmostly.sdata)
		*(*.sdata)
		. = ALIGN(8);

		PROVIDE(_sdata_end = .);
	}

	. = ALIGN(0x1000); */

	.bss :
	{
		PROVIDE(_bss_start = .);
		*(.bss)
		*(.bss.*)

		*(.sbss)
		*(.sbss.*)

		. = ALIGN(8);
		PROVIDE(_bss_end = .);
	}

	/* End of the read-write data sections */

	. = ALIGN(0x1000); /* Need this to create proper sections */

	/* .sbss :
	{
		PROVIDE(_sbss_start = .);
		*(.sbss)
		*(.sbss.*)
		. = ALIGN(8);
		PROVIDE(_sbss_end = .);
	}

	. = ALIGN(0x1000); */

	PROVIDE(_kernel_end = .);

	/* Provide heap end  */
	__heap_start__ = .;
    end = __heap_start__;


	/* . = .+ _KERNEL_STACK_SIZE; */

	/* .heap : {
        
        _end = end;
        __end = end;
        KEEP(*(.heap))
        __heap_end__ = .;
        __HeapLimit = __heap_end__;
    } */

}
