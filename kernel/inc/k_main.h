
#ifndef __K_MAIN_H__
#define __K_MAIN_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <reent.h>

#include "llenv.h"
    enum k_stage_t
    {
        K_BEFORE_BOOT = 0,
        K_BOOT = 1,
        K_BOOT_PERIP = 2,
        K_BOOT_HARTS = 3,
        K_MULTICORE = 4,
        K_CLEARUP = 5
    };

    extern volatile k_stage_t k_stage;
    extern bool k_stdout_switched;
    extern volatile unsigned long k_cpuclock;

    extern thread_local _reent hl_reent;
    extern thread_local int hartid;
    extern thread_local volatile void *k_local_resume;

    int k_boot(void **);
    int k_boot_perip();
    int k_boot_harts(int);

    int k_premain(int hartid);
    int k_main(int hartid);
    int k_after_main(int, int);

#ifdef __cplusplus
}

#include <functional>
#include <atomic>
extern std::function<int(const char *, int size)> k_stdout_func;

#include "k_sysdev.h"
#include "k_mmu.h"
#include "k_vmmgr.hpp"

extern std::atomic_int k_hart_state[K_CONFIG_MAX_PROCESSORS];
extern SysRoot *sysroot;
extern SysCPU *syscpu;
extern SysMem *sysmem;
extern MMUBase *sysmmu;
extern VMemoryMgr *sysvmm;
#endif

#endif