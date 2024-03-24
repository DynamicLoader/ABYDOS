#include <atomic>

#include "k_main.h"

std::atomic_flag k_malloc_lock = ATOMIC_FLAG_INIT;

extern "C"
{

#include <sys/lock.h>

    // Hook with libc
    int _write(int fd, char *buf, int size)
    {
        if (k_stdout_switched)
            return k_stdout_func(buf, size);
        sbi_ecall(SBI_EXT_DBCN, SBI_EXT_DBCN_CONSOLE_WRITE, size, (unsigned long)buf, 0, 0, 0, 0);
        return size;
    }

    struct _reent *__getreent(void)
    {
        _write(0, (char*)"[_] ", 4);
        if (k_stage == K_MULTICORE)
            return &hl_reent;
        return _GLOBAL_REENT;
    }

    void __malloc_lock(struct _reent *reent)
    {
        // _write(0,(char*)"mlock\n",6);
        if (k_stage != K_MULTICORE)
            return;
        while (k_malloc_lock.test_and_set())
            ;
    }

    void __malloc_unlock(struct _reent *reent)
    {
        // _write(0,(char*)"munlock\n",8);
        if (k_stage != K_MULTICORE)
            return;
        k_malloc_lock.clear();
    }

    /**
        Locks - Redefined.
     */
#if !defined(_RETARGETABLE_LOCKING)
#warning Retargetable locking is not enabled. This may result in improper behavior!

#else
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

    struct __lock __lock___sinit_recursive_mutex;
    struct __lock __lock___sfp_recursive_mutex;
    struct __lock __lock___atexit_recursive_mutex;
    struct __lock __lock___at_quick_exit_mutex;
    struct __lock __lock___malloc_recursive_mutex;
    struct __lock __lock___env_recursive_mutex;
    struct __lock __lock___tz_mutex;
    struct __lock __lock___dd_hash_mutex;
    struct __lock __lock___arc4random_mutex;

    void __retarget_lock_init(_LOCK_T *lock_ptr)
    {
        // _write(0, (char *)"__retarget_lock_init\n", 21);
        *lock_ptr = new __lock();
    }

    void __retarget_lock_init_recursive(_LOCK_T *lock_ptr)
    {
        // _write(0, (char *)"__retarget_lock_init_recursive\n", 31);
        *lock_ptr = new __lock();
    }

    void __retarget_lock_close(_LOCK_T lock)
    {
        // _write(0, (char *)"__retarget_lock_close\n", 22);
        delete lock;
    }

    void __retarget_lock_close_recursive(_LOCK_T lock)
    {
        // _write(0, (char *)"__retarget_lock_close_recursive\n", 32);
        delete lock;
    }

    void __retarget_lock_acquire(_LOCK_T lock)
    {
        // _write(0, (char *)"__retarget_lock_acquire\n", 24);
        lock->lock();
    }

    void __retarget_lock_acquire_recursive(_LOCK_T lock)
    {
        lock->lock(true);
        // _write(0, (char *)"__retarget_lock_acquire_recursive\n", 34);
    }

    int __retarget_lock_try_acquire(_LOCK_T lock)
    {
        // _write(0, (char *)"__retarget_lock_try_acquire\n", 28);
        if (k_stage != K_MULTICORE)
            return 1;
        int null_owner = -1;
        if (lock->owner.compare_exchange_weak(null_owner, hartid))
        {
            lock->recursive_count = 1;
            return 1;
        }
        return 0;
    }

    int __retarget_lock_try_acquire_recursive(_LOCK_T lock)
    {
        if (k_stage != K_MULTICORE)
            return 1;
        if (lock->owner == hartid)
        {
            lock->recursive_count++;
            return 1;
        }
        int null_owner = -1;
        if (lock->owner.compare_exchange_weak(null_owner, hartid))
        {
            lock->recursive_count = 1;
            return 1;
        }
        return 0;
    }

    void __retarget_lock_release(_LOCK_T lock)
    {
        lock->unlock();
        // _write(0, (char *)"__retarget_lock_release\n", 24);
    }

    void __retarget_lock_release_recursive(_LOCK_T lock)
    {
        lock->unlock(true);
        // _write(0, (char *)"__retarget_lock_release_recursive\n", 34);
    }

#endif // _RETARGETABLE_LOCKING

} // extern "C"