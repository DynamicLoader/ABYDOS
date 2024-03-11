
#ifndef __LLENV_H__
#define __LLENV_H__

#include <sbi/sbi_ecall_interface.h>
#include <sbi/riscv_encoding.h>
#include "riscv_asm.h"
#include <stdint.h>


#define K_FDT_MAX_SIZE 65536

struct sbiret {
  long error;
  long value;
};

struct sbiret sbi_ecall(int ext, int fid, unsigned long arg0,
                        unsigned long arg1, unsigned long arg2,
                        unsigned long arg3, unsigned long arg4,
                        unsigned long arg5);

extern uint8_t k_fdt[K_FDT_MAX_SIZE];
extern uint64_t k_mem_size;

#endif