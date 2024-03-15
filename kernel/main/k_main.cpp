
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

int k_premain_0(void **sys_stack_base)
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
        std::cout << "====================" << std::endl;
    }
    sysmmu = new SV39MMU(0);

    std::cout << "Enabling MMU..." << std::flush;
    if (!sysmmu->enable(true))
    {
        std::cout << "Failed!" << std::endl;
        return K_EFAIL;
    }
    std::cout << "OK!" << std::endl;

    // int a = 0;
    // printf("Original addr of a: 0x%lx with value %i\n", (uintptr_t)&a, a);

    // int *pa = (int*)((((uintptr_t)(&a)) | 0xFFFFFFC000000000) + 0x80000000);

    // for(int i = 0;i< 256;++i){
    //     printf("Mapped addr of a: 0x%lx\n", (uintptr_t)pa);
    //     printf("Mapped value of a: %i\n", *pa);
    //     *pa += 1;
    //     pa += 0x40000000 / sizeof(int);
    // }

    // printf("We modified from mapped, now the original value is %i\n", a);

    // Setup stack (SV39)
    *sys_stack_base = (void *)((uintptr_t)1 << 38);
    // **(ulong**)(sys_stack_base - 8) = 0x123456789;

    printf("Preparing to set SP: 0x%lx\n", (uintptr_t)*sys_stack_base);

    return 0;
}

int k_main(int hartid)
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
                return uart->write(stdouthdl, buf, size);
            };
            k_stdout_switched = true;
            std::cout << "Hello from local driver!" << std::endl;
        }
    }

    // auto ret = SBIF::SRST::reset(SBIF::SRST::WarmReboot, SBIF::SRST::NoReason);
    // std::cout << "Reset: " << SBIF::getErrorStr(ret) << std::endl;
    return 0;
}

// Hook with libc
extern "C" int _write(int fd, char *buf, int size)
{
    if (k_stdout_switched)
        return k_stdout_func(buf, size);
    sbi_ecall(SBI_EXT_DBCN, SBI_EXT_DBCN_CONSOLE_WRITE, size, (unsigned long)buf, 0, 0, 0, 0);
    return size;
}