
#include <iostream>
#include <string>
#include <functional>
#include <atomic>

#include "k_main.h"
#include "k_sbif.hpp"
#include "k_drvif.h"
#include "k_sysdev.h"
#include "k_mem.hpp"
#include "k_vmmgr.hpp"

std::atomic_int k_hart_state[K_CONFIG_MAX_PROCESSORS] = {0};

thread_local _reent hl_reent;
thread_local int hartid;
thread_local volatile bool k_halt = false;

thread_local volatile void *k_local_resume = nullptr;

// to be run by each hart
int k_premain(int hartid)
{
    if (k_hart_state[hartid] != 1)
    { // Not desired to be bootup, maybe timeout, or failed
        SBIF::HSM::stopHart();
        while (1)
            wfi();
    }

    _REENT_INIT_PTR(&hl_reent);
    ::hartid = hartid;

    // Setup Hart Timer
    auto rc = SBIF::Timer::setTimer(hartid * k_cpuclock);
    if (rc)
    {
        printf("Failed to set timer: %ld\n", rc);
        k_hart_state[hartid] = 3; // mark as failed
        SBIF::HSM::stopHart();
        while (1)
            wfi();
    }
    extern char _exctable;
    set_stvec((unsigned long)&_exctable, 1);
    csr_set(CSR_SIE, SIP_STIP | SIP_SSIP);

    k_hart_state[hartid] = 2;
    while (k_stage != K_MULTICORE)
        ; // wait for the boot core to finish
    csr_set(CSR_SSTATUS, SSTATUS_SIE);
    return hartid; // keep hart id in a0
}

void runUserAPP()
{
    extern char __load_start_umode1, __load_stop_umode1;
    if (!k_local_resume)
    {
        size_t dlen = &__load_stop_umode1 - &__load_start_umode1;
        size_t len = dlen & 0xFFF ? (dlen & ~0xFFFULL) + 0x1000 : dlen;
        auto dst = alignedMalloc<uint8_t>(len, 4096);
        memcpy(dst, &__load_start_umode1, dlen);
        // For test now, we'd copy a mmu table with different ASID to support Task Scheduling
        auto runaddr = sysmmu->getVMAUpperBottom();
        // sysmmu->map(runaddr, (uintptr_t)dst, len,
        //             MMUBase::PROT_R | MMUBase::PROT_W | MMUBase::PROT_X | MMUBase::PROT_U);
        // sysmmu->apply();
        auto vmm = new VMemoryMgr(sysmmu->fork(hartid));
        vmm->addMap(runaddr, (uintptr_t)dst, len,
                    MMUBase::PROT_R | MMUBase::PROT_W | MMUBase::PROT_X | MMUBase::PROT_U);
        vmm->confirm();
        vmm->getMMU()->switchASID();
        vmm->getMMU()->apply();

        // disable interrupt for now
        csr_write(CSR_SSTATUS, (csr_read(CSR_SSTATUS) | SSTATUS_SUM) & ~SSTATUS_SIE &
                                   ~SSTATUS_SPP); // permit read U-mode page from S-mode
        k_local_resume = &&_resume;
        asm volatile("csrw sscratch, sp \n" //  save sp in sscratch
                     "csrw sepc, %0 \n"     //  set sepc to the entry
                     "csrc sstatus, %1 \n"  //  clear SPP
                     "sret"                 // switch to user mode
                     ::"r"(runaddr),
                     "r"(SSTATUS_SPP)
                     : "memory");
    }

_resume:
    asm volatile("csrw sscratch, zero \n" //  clear sscratch
    );
    printf("Resume from U-mode!");
}

int k_main(int hartid)
{

    printf("Hello from hart %d!\n", hartid);
    // asm volatile("ecall \n");
    while (!k_halt)
        runUserAPP();

    // printf("Hart %d is running!\n", hartid);
    return 0;
}

int k_after_main(int hartid, int main_ret)
{
    // ebreak();
    csr_clear(CSR_SIE, (uint64_t)-1);
    if (hartid < 0) // Negitive hartid for non-boot hart
    {

        printf("Hart %i has returned with %d\n", ::hartid, main_ret);
        k_hart_state[::hartid] = 3;
        printf("Failed to stop hart: %ld\n", SBIF::HSM::stopHart());
    }
    else
    {
        printf("\n> Waiting for other harts to return...\n");
        int flag = 0;
        do // a timeout can be added here
        {
            flag = 0;
            for (int i = 0; i < K_CONFIG_MAX_PROCESSORS; ++i)
            {
                if (i == hartid)
                    continue;
                if (k_hart_state[i] == 1 || k_hart_state[i] == 2)
                {
                    flag = 1;
                    break;
                }
                if (k_hart_state[i] == 3 || k_hart_state[i] == 0)
                    continue;
            }
        } while (flag);
        k_stdout_switched = false;
        k_stage = K_CLEARUP;
    }
    return main_ret; // pass to the lower
}
