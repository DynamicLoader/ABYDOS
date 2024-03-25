
#include <iostream>
#include <string>
#include <functional>
#include <atomic>

#include "k_main.h"
#include "k_sbif.hpp"
#include "k_drvif.h"
#include "k_sysdev.h"
#include "k_mmu.h"

SBIF::DebugCon dbg;

std::function<int(const char *, int size)> k_stdout_func;
bool k_stdout_switched = false;

SysRoot *sysroot = nullptr;
SysMem *sysmem = nullptr;
MMUBase *sysmmu = nullptr;

// Only write from boot core
k_stage_t k_stage = K_BEFORE_BOOT;

std::atomic_int k_hart_state[8] = {0};

int k_boot(void **sys_stack_base)
{
    k_stage = K_BOOT;
#ifdef DEBUG
    std::cout << "Dumping Device tree..." << std::endl;
    fdt_print_node(k_fdt, 0, 0);
#endif

    std::cout << "> Probing system devices" << std::endl;
    DriverManager::probe(k_fdt, DEV_TYPE_SYS);

    auto srhdl = DriverManager::getDrvByPath(k_fdt, "/", (void **)&sysroot);
    if (srhdl < 0)
    {
        std::cout << "[E] SysRoot not installed properly... Kernel Panic!" << std::endl;
        return K_ENOSPC;
    }
    else
    {
        std::cout << "====================" << std::endl;
        std::cout << "Model: " << sysroot->model() << std::endl;
        std::cout << "Compatible: " << sysroot->compatible() << std::endl;
        std::cout << "Stdout: " << sysroot->stdout_path() << std::endl;
        std::cout << "Bootargs: " << sysroot->bootargs() << std::endl;
        std::cout << "====================" << std::endl;
    }

    auto smhdl = DriverManager::getDrvByPath(
        k_fdt, "/memory",
        (void **)&sysmem); // must found at least one memory node, otherwise the kernel does not run here!
    if (smhdl < 0)         // We also judge here to see if the driver is configured properly...
    {
        std::cout << "[E] SysMem not installed properly... Kernel Panic!" << std::endl;
        return K_ENOSPC;
    }
    else
    {
        // Add reserved memory from DRAM_START to KERNEL_OFFSET
        extern char _DRAM_BASE, _KERNEL_OFFSET;
        sysmem->addReservedMem((size_t)&_DRAM_BASE, (size_t)&_KERNEL_OFFSET);
        std::cout << "Reserved memory: ";
        for (auto &mem : sysmem->reservedMem())
            std::cout << "(0x" << std::hex << mem.first << " - 0x" << mem.first + mem.second << ") ";
        std::cout << std::endl;

        std::cout << "Available memory: ";
        for (auto &mem : sysmem->availableMem())
            std::cout << "(0x" << std::hex << mem.first << " - 0x" << mem.first + mem.second << ") ";
        std::cout << std::endl;
        std::cout << "====================" << std::endl << std::dec;
    }

    // Setup MMU
    auto rc = 0;
    sysmmu = new SV39MMU(-1);
    // Lower 4G: Direct mapping
    rc =
        sysmmu->map(0, 0, 4 * 1024 * 1048576ULL, MMUBase::PROT_R | MMUBase::PROT_W | MMUBase::PROT_X | MMUBase::PROT_G);
    if (rc < 0)
    {
        std::cout << "[E] Failed to map lower 4G: " << rc << std::endl;
        return K_EFAIL;
    }

    std::cout << "Enabling MMU..." << std::flush;
    if (!sysmmu->enable(true))
    {
        std::cout << "Failed!" << std::endl;
        return K_EFAIL;
    }
    std::cout << "OK!" << std::endl;

    // auto a = alignedMalloc<long>(4096, 4096);
    // printf("Original addr of a: 0x%lx\n", (uintptr_t)a);
    // *a = 1145141919810;
    // printf("Original value of a: %li\n", *a);
    // sysmmu->map((uintptr_t)a | 0xFFFFFFC000000000, (uintptr_t)a, 4096, MMUBase::PROT_W | MMUBase::PROT_R);
    // sysmmu->apply();

    // printf("Mapped addr of a: 0x%lx\n", (uintptr_t)a | 0xFFFFFFC000000000);
    // printf("Mapped value of a: %li\n", *(long *)((uintptr_t)a | 0xFFFFFFC000000000));
    // *(long *)((uintptr_t)a | 0xFFFFFFC000000000) = 1919810114514;
    // printf("We modified from mapped, now the original value is %li\n", *a);

    size_t boot_stack_size = K_CONFIG_KERNEL_STACK_SIZE;
    auto kstack = alignedMalloc<void>(boot_stack_size, 4096);
    rc = sysmmu->map(sysmmu->getVMALowerTop() - boot_stack_size, (uintptr_t)kstack, boot_stack_size,
                     MMUBase::PROT_R | MMUBase::PROT_W | MMUBase::PROT_X | MMUBase::PROT_G);
    if (rc < 0)
    {
        std::cout << "[E] Failed to map global kernel stack: " << rc << std::endl;
        return K_EFAIL;
    }
    sysmmu->apply();

    *sys_stack_base = (void *)sysmmu->getVMALowerTop();
    printf("> Preparing to set SP: 0x%lx\n", (uintptr_t)*sys_stack_base);
    return 0;
}

int k_boot_perip()
{
    k_stage = K_BOOT_PERIP;
    // Probing peripheral devices
    std::cout << "> Probing peripheral devices" << std::endl;
    DriverManager::probe(k_fdt, DEV_TYPE_PERIP);

    if (sysroot->stdout_path().empty())
    {
        std::cout << "[W] Stdout not specified!" << std::endl;
    }
    else
    {
        DriverChar *uart;
        auto stdouthdl = DriverManager::getDrvByPath(k_fdt, sysroot->stdout_path().c_str(), (void **)&uart);
        if (stdouthdl < 0)
        {
            std::cout << "[W] Stdout specified but not installed properly!" << std::endl;
        }
        else
        {
            std::cout << "Switching stdout to " << sysroot->stdout_path() << std::endl;
            k_stdout_func = [stdouthdl, uart](const char *buf, int size) -> int {
                // uart->write(stdouthdl, "- ", 2);
                return uart->write(stdouthdl, buf, size);
            };
            k_stdout_switched = true;
            std::cout << "Hello from local driver!" << std::endl;
        }
    }

    return 0;
}

int k_boot_harts(int boot_hartid)
{
    k_stage = K_BOOT_HARTS;
    extern char _start_hart;
    printf("> Booting harts...\n");
    for (int i = 0; i < 8; i++)
    {
        if (i == boot_hartid)
            continue;
        auto rc = SBIF::HSM::getHartState(i);
        if (rc < 0) // invalid core, skip
            continue;
        if (rc == SBI_HSM_STATE_STOPPED)
        {
            printf("Starting hart %d...", i);
            fflush(stdout);
            k_hart_state[i] = 1; // preset to 1
            rc = SBIF::HSM::startHart(i, (uintptr_t)&_start_hart, 0);
            if (rc < 0)
            {
                std::cout << "[E] Failed to start hart " << i << ": " << SBIF::getErrorStr(rc) << std::endl;
                k_hart_state[i] = 0; // reset to 0
                continue;
            }
            while (k_hart_state[i] < 2)
                ; // timeout here!
            printf("OK!\n");
        }
        else
        {
            printf("Hart %d is in a state of %ld\n", i, rc);
        }
    }
    k_hart_state[boot_hartid] = 1;
    printf("> Switching to multicore mode\n");
    k_stage = K_MULTICORE;
    return 0;
}

thread_local _reent hl_reent;
thread_local int hartid;

// to be run by each hart
int k_premain(int hartid)
{

    _REENT_INIT_PTR(&hl_reent);
    ::hartid = hartid;

    // Setup Hart Timer
    auto rc = SBIF::Timer::setTimer(hartid * 10000000);
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
        ;          // wait for the boot core to finish
    csr_set(CSR_SSTATUS, SSTATUS_SIE);
    return hartid; // keep hart id in a0
}

thread_local int magic = 114514;

int k_main(int hartid)
{

    printf("Hello from hart %d! %i\n", hartid, magic);
    while(1);
        // printf("Hart %d is running!\n", hartid);
    return 0;
}

int k_after_main(int hartid, int main_ret)
{
    csr_clear(CSR_SIE, (uint64_t)-1);
    if (hartid < 0) // Negitive hartid for non-boot hart
    {

        printf("Hart %i has returned with %d\n", -hartid, main_ret);
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
            for (int i = 0; i < 8; ++i)
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

class Test
{
  public:
    static void test() __attribute__((interrupt("supervisor")))
    {
        printf("Hello from Test!\n");
    }
};

// __attribute__((naked)) void isr_test_entry()
// {
//     asm volatile("
//         csrrw sp, sscratch, sp;

//     ");

//     Test::test();

//     asm volatile("sret");
// }
