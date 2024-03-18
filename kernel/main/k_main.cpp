
#include <iostream>
#include <string>
#include <functional>

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

MMUBase *hartmmu[8] = {nullptr};

int k_boot(void **sys_stack_base)
{
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
    sysmmu = new SV39MMU(0);
    // Lower 4G: Direct mapping
    rc = sysmmu->map(0, 0, 4 * 1024 * 1048576ULL, MMUBase::PROT_R | MMUBase::PROT_W | MMUBase::PROT_X);
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

    // Create mmu for each hart
    for (int i = 0; i < 8; i++)
    {
        hartmmu[i] = new SV39MMU(i);
        auto rc = hartmmu[i]->map(0, 0, 4 * 1024 * 1048576ULL,
                                  MMUBase::PROT_R | MMUBase::PROT_W | MMUBase::PROT_X |
                                      MMUBase::PROT_G); // Should be cloned from sysmmu, test here
        if (rc)
        {
            std::cout << "[E] Failed to map lower 4G for hart " << i << ": " << rc << std::endl;
            return K_EFAIL;
        }
        auto kstack = alignedMalloc<void>(K_CONFIG_STACK_SIZE, 4096);
        rc = hartmmu[i]->map(hartmmu[i]->getVMALowerTop() - K_CONFIG_STACK_SIZE, (uintptr_t)kstack, K_CONFIG_STACK_SIZE,
                             MMUBase::PROT_R | MMUBase::PROT_W | MMUBase::PROT_X);
        if (rc)
        {
            std::cout << "[E] Failed to map kernel stack for hart " << i << ": " << rc << std::endl;
            return K_EFAIL;
        }
        // Here we do not enable MMU for harts, as they will be enabled by themselves
    }

    extern char _KERNEL_BOOT_STACK_SIZE;
    size_t boot_stack_size = (size_t)&_KERNEL_BOOT_STACK_SIZE;
    auto kstack = alignedMalloc<void>(boot_stack_size, 4096);
    rc = sysmmu->map(sysmmu->getVMALowerTop() - boot_stack_size, (uintptr_t)kstack, boot_stack_size,
                     MMUBase::PROT_R | MMUBase::PROT_W | MMUBase::PROT_X);
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

    // Prepare sysmmu for a global kernel stack
    // extern char _KERNEL_BOOT_STACK_SIZE;
    // size_t boot_stack_size = (size_t)&_KERNEL_BOOT_STACK_SIZE;
    // sysmmu->unmap(sysmmu->getVMALowerTop() - boot_stack_size, boot_stack_size);

    return 0;
}

int k_boot_harts(int boot_hartid)
{
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
            printf("Starting hart %d...\n", i);
            unsigned long satp;
            hartmmu[i]->getSATP(&satp);
            rc = SBIF::HSM::startHart(i, (uintptr_t)&_start_hart, satp);
            if (rc < 0)
            {
                std::cout << "[E] Failed to start hart " << i << ": " << SBIF::getErrorStr(rc) << std::endl;
                return K_EFAIL;
            }
        }
        else
        {
            printf("Hart %d is in a state of %ld\n", i, rc);
        }
        // std::cout << "Hart #" << i << " state: " << SBIF::HSM::getHartState(i) << std::endl;
    }
    return 0;
}

// to be run by each hart
int k_main(int hartid)
{
    printf("Hello from hart %d!\n", hartid);
    // auto ret = SBIF::SRST::reset(SBIF::SRST::WarmReboot, SBIF::SRST::NoReason);
    // std::cout << "Reset: " << SBIF::getErrorStr(ret) << std::endl;
    return 0;
}

int k_after_main(int hartid, int main_ret)
{
    if (hartid < 0)
    {
        printf("A hart has returned with %d\n", main_ret);
        printf("Failed to stop hart: %ld\n", SBIF::HSM::stopHart());
    }
    else
    {
        printf("\n> Waiting for other harts to return...\n");
        for (int flag = 0; flag;) // a timeout can be added here
        {
            for (int i = 0; i < 8; ++i)
            {
                if (i == hartid)
                    continue;
                auto rc = SBIF::HSM::getHartState(i);
                if (rc < 0) // invalid core, skip
                    continue;
                if (rc != SBI_HSM_STATE_STOPPED)
                {
                    flag = 1;
                }
            }
        }
        k_stdout_switched = false;
    }
    return main_ret; // pass to the lower
}

// Hook with libc
extern "C" int _write(int fd, char *buf, int size)
{
    if (k_stdout_switched)
        return k_stdout_func(buf, size);
    sbi_ecall(SBI_EXT_DBCN, SBI_EXT_DBCN_CONSOLE_WRITE, size, (unsigned long)buf, 0, 0, 0, 0);
    return size;
}