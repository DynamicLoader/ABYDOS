
#ifndef __LLENV_H__
#define __LLENV_H__

#include <sbi/sbi_ecall_interface.h>
#include <sbi/riscv_encoding.h>
#include "riscv_asm.h"
#include <stdint.h>
#include <reent.h>

#include "k_defs.h"

struct sbiret
{
    long error;
    long value;
};

// At most _KERNEL_HART_LOCAL_DATA_SIZE bytes
struct hartLocal_t
{
    char _exceptionBackup[K_CONFIG_EXCEPTION_BACKUP_SIZE];
    struct _reent reent;
};

struct sbiret sbi_ecall(int ext, int fid, unsigned long arg0, unsigned long arg1, unsigned long arg2,
                        unsigned long arg3, unsigned long arg4, unsigned long arg5);

// extern uint8_t k_fdt[K_FDT_MAX_SIZE];
extern void *k_fdt;

// void *k_getHartLocal();

#endif