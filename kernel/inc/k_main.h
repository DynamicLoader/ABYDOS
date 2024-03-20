
#ifndef __K_MAIN_H__
#define __K_MAIN_H__



#ifdef __cplusplus

#include <functional>
extern std::function<int(const char *, int size)> k_stdout_func;

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

    extern k_stage_t k_stage;
    extern bool k_stdout_switched;

    extern thread_local _reent hl_reent;
    extern thread_local int hartid;

    int k_boot(void **);
    int k_boot_perip();
    int k_boot_harts(int);

    int k_premain(int hartid);
    int k_main(int hartid);
    int k_after_main(int, int);

#ifdef __cplusplus
}
#endif

#endif