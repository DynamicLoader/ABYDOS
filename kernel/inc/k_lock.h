#ifndef __K_LOCK_H__
#define __K_LOCK_H__

#include <atomic>
#include "k_main.h"

struct __lock
{
    std::atomic<int> owner = -1;
    std::atomic<int> recursive_count = 0;

    void lock(bool recursive = false)
    {

        if (k_stage != K_MULTICORE)
            return;
        // _write(0, (char*)"PLOCK\n",6);
        if (recursive)
        {
            if (owner == hartid)
            {
                recursive_count++;
                return;
            }
        }
        int null_owner = -1; // helper for compare_exchange_weak
        while (!owner.compare_exchange_weak(null_owner, hartid))
            null_owner = -1;

        recursive_count = 1;
        // _write(0, (char *)"LOCK\n", 5);
    }

    void unlock(bool recursive = false)
    {
        if (k_stage != K_MULTICORE)
            return;
        if (recursive)
        {
            if (owner == hartid)
            {
                if (--recursive_count == 0)
                    owner = -1;
            }
            return;
        }

        if (owner == hartid)
            owner = -1;
    }
};


using lock_t = __lock;

#endif