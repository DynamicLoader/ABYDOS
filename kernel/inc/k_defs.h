#ifndef __K_DEFS_H__
#define __K_DEFS_H__


// CONFIG

// #define K_CONFIG_EXCEPTION_BACKUP_SIZE  512
#define K_CONFIG_MAX_PROCESSORS  64
#define K_CONFIG_STACK_SIZE_N 13
#define K_CONFIG_STACK_SIZE (1 << K_CONFIG_STACK_SIZE_N) // 8K, must be 2^n bytes
#define K_CONFIG_KERNEL_STACK_SIZE  K_CONFIG_MAX_PROCESSORS * K_CONFIG_STACK_SIZE
#define K_CONFIG_CPU_DEFAULT_CLOCK 10000000

#define K_CONFIG_DEFAULT_SCHEDULER "scheduler-rr"

// END CONFIG


#ifndef __ASSEMBLER__

#include "sbi/sbi_ecall_interface.h"


#ifdef __cplusplus
#define K_ISR extern "C"
#else
#define K_ISR 
#endif
#define K_ISR_ENTRY extern "C" __attribute__((naked)) 

// Priority of the driver and global constructors
#define K_PR_INIT_DRV_LIST 101

#define K_PR_DRV_SYSROOT_BEGIN 110
#define K_PR_DRV_SYSROOT_END 115

#define K_PR_DEV_SYSMEM_BEGIN 116
#define K_PR_DEV_SYSMEM_END 120

#define K_PR_DEV_SYSCPU_BEGIN 121
#define K_PR_DEV_SYSCPU_END 125

#define K_PR_DEV_SYSSCHED_BEGIN 126
#define K_PR_DEV_SYSSCHED_END 130


#define K_OK 0
#define K_EFAIL SBI_ERR_FAILED
#define K_ENOTSUPP SBI_ERR_NOT_SUPPORTED
#define K_EINVAL SBI_ERR_INVALID_PARAM
#define K_EDENIED SBI_ERR_DENIED
#define K_EINVALID_ADDR SBI_ERR_INVALID_ADDRESS
#define K_EALREADY SBI_ERR_ALREADY_AVAILABLE
#define K_EALREADY_STARTED SBI_ERR_ALREADY_STARTED
#define K_EALREADY_STOPPED SBI_ERR_ALREADY_STOPPED
#define K_ENO_SHMEM SBI_ERR_NO_SHMEM

#define K_ENODEV -1000
#define K_ENOSYS -1001
#define K_ETIMEDOUT -1002
#define K_EIO -1003
#define K_EILL -1004
#define K_ENOSPC -1005
#define K_ENOMEM -1006
#define K_EUNKNOWN -1007
#define K_ENOENT -1008

#endif // __ASSEMBLER__

#endif