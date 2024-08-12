
#include <iostream>
#include <string>
#include <functional>
#include <atomic>
#include <fstream>

#include "k_main.h"
#include "k_sbif.hpp"
#include "k_drvif.h"
#include "k_sysdev.h"
#include "k_mem.hpp"
#include "k_vmmgr.hpp"
#include "k_vfs.h"

thread_local _reent hl_reent;
thread_local int hartid;
thread_local volatile bool k_halt = false;
thread_local volatile void *k_local_resume = nullptr;

// to be run by each hart
int k_pre_main(int hartid)
{
    if (k_hart_state[hartid] != 1) // Not desired to be bootup, maybe timeout, or failed
        return K_ENOSPC;

    // Test Hart Timer
    auto rc = SBIF::Timer::setTimer(0);
    if (rc)
    {
        printf("Failed to set timer: %ld\n", rc);
        k_hart_state[hartid] = 3; // mark as failed
        return K_EFAIL;
    }

    // Init hart locals
    _REENT_INIT_PTR(&hl_reent);
    ::hartid = hartid;

    // set up exception table
    extern char _exctable;
    set_stvec((unsigned long)&_exctable, 1);
    csr_set(CSR_SIE, SIP_STIP | SIP_SSIP); // This cannot be moved to main or other place, not known why

    k_hart_state[hartid] = 2;
    while (k_stage != K_MULTICORE)
        ; // wait for the boot core to finish
    csr_set(CSR_SSTATUS, SSTATUS_SIE);
    return 0;
}

// void runInUserMode(uintptr_t runaddr)
// {
//     // disable interrupt for now
//     csr_write(CSR_SSTATUS, (csr_read(CSR_SSTATUS) | SSTATUS_SUM) & ~SSTATUS_SIE &
//                                ~SSTATUS_SPP); // permit read U-mode page from S-mode
//     k_local_resume = &&_resume;

//     asm volatile("csrw sscratch, sp \n" //  save sp in sscratch
//                  "csrw sepc, %0 \n"     //  set sepc to the entry
//                  "csrc sstatus, %1 \n"  //  clear SPP
//                  ::"r"(runaddr),
//                  "r"(SSTATUS_SPP)
//                  : "memory");
//     /* Switch to U-mode, while saving the context by compiler */
//     asm volatile("sret" ::
//                      : "memory", "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7", "t0", "t1", "t2", "t3", "t4", "t5",
//                        "t6", "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11",
//                        "ra" /* sp, gp and tp are restored in ISR */);

// _resume:
//     printf("Resume from U-mode!");
// }

// void runUserAPP()
// {
//     extern char __load_start_umode1, __load_stop_umode1;
//     if (!k_local_resume)
//     {
//         size_t dlen = &__load_stop_umode1 - &__load_start_umode1;
//         size_t len = dlen & 0xFFF ? (dlen & ~0xFFFULL) + 0x1000 : dlen;
//         auto dst = alignedMalloc<uint8_t>(len, 4096);
//         memcpy(dst, &__load_start_umode1, dlen);
//         // For test now, we'd copy a mmu table with different ASID to support Task Scheduling
//         auto runaddr = sysmmu->getVMAUpperBottom();
//         // sysmmu->map(runaddr, (uintptr_t)dst, len,
//         //             MMUBase::PROT_R | MMUBase::PROT_W | MMUBase::PROT_X | MMUBase::PROT_U);
//         // sysmmu->apply();
//         auto vmm = new VMemoryMgr(sysmmu->fork(hartid));
//         vmm->addMap(runaddr, (uintptr_t)dst, len,
//                     MMUBase::PROT_R | MMUBase::PROT_W | MMUBase::PROT_X | MMUBase::PROT_U);
//         vmm->confirm();
//         vmm->getMMU()->switchASID();
//         vmm->getMMU()->apply();

//         runInUserMode(runaddr);
//     }
// }

/**
 * @brief Hart main function
 *
 * @return int errno
 * @note Global interrupt should NOT be operated inside the function
 */
int k_main()
{
    printf("Hello from hart %d!\n", hartid);

    std::ifstream f("/2.txt");
    if(f.is_open())
    {
        std::string line;
        while(getline(f, line))
        {
            printf("!!!Reading from 2.txt: %s\n", line.c_str());
        }
        f.close();
    }
    else
    {
        printf("Failed to open file\n");
    }


    // while (!k_halt)
    //     runUserAPP();

    return 0;
}

int k_after_main(int main_ret)
{
    // Disable all interrupts
    csr_clear(CSR_SIE, (uint64_t)-1);
    printf("Hart %i has returned with %d\n", ::hartid, main_ret);
    while (k_hart_state[::hartid] != 3)
        k_hart_state[::hartid] = 3;
    return main_ret; // pass to the lower
}
