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
#include "k_vfs.h"

std::vector<VMemoryMgr::map_t> VMemoryMgr::_global_maps;

std::function<int(const char *, int size)> k_stdout_func;
bool k_stdout_switched = false;

SysRoot *sysroot = nullptr;
SysCPU *syscpu = nullptr;
SysMem *sysmem = nullptr;
MMUBase *sysmmu = nullptr;
VMemoryMgr *sysvmm = nullptr;

volatile unsigned long k_cpuclock = 0;

// Only write from boot core
volatile k_stage_t k_stage = K_BEFORE_BOOT;
std::atomic_int k_hart_state[K_CONFIG_MAX_PROCESSORS] = {0};

int k_boot(void **sys_stack_base)
{
    k_stage = K_BOOT;
#ifdef DEBUG
    std::cout << "Dumping Device tree..." << std::endl;
    fdt_print_node(k_fdt, 0, 0);
#endif

    // Probe SBI extensions
    std::cout << "> Listing SBI and Machine info" << std::endl;
    std::cout << "Machine Vendor ID : " << SBIF::Base::getVID() << std::endl;
    std::cout << "Machine Arch ID   : " << SBIF::Base::getAID() << std::endl;
    std::cout << "Machine Impl ID   : " << SBIF::Base::getMacImplID() << std::endl;
    auto sbispecv = SBIF::Base::getSpecVersion();
    std::cout << "SBI Spec Version  : " << ((sbispecv & 0x7F000000) >> 24) << "." << (sbispecv & 0xFFFFFF) << std::endl;
    auto sbiimpl = SBIF::Base::getImplID();
    std::cout << "SBI Impl ID       : " << sbiimpl << "(" << SBIF::Base::getImplName(sbiimpl) << ")" << std::endl;
    std::cout << "SBI Impl Version  : " << SBIF::Base::getImplVer() << std::endl;
    std::cout << "SBI Extensions    : [ ";
    if (SBIF::Timer::available())
        std::cout << " TIME ";
    if (SBIF::IPI::available())
        std::cout << " IPI ";
    if (SBIF::HSM::available())
        std::cout << " HSM ";
    if (SBIF::DBCN::available())
        std::cout << " DBCN ";
    if (SBIF::SRST::available())
        std::cout << " SRST ";
    if (SBIF::RFNC::available())
        std::cout << " RFNC ";
    std::cout << " ]" << std::endl;

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

    auto scpuhdl = DriverManager::getDrvByPath(k_fdt, "/cpus", (void **)&syscpu);
    if (scpuhdl < 0)
    {
        std::cout << "[E] SysCPU not installed properly... Kernel Panic!" << std::endl;
        return K_ENOSPC;
    }
    else
    {
        auto cpus = syscpu->CPUs();
        std::cout << "CPU Timebase frequency: " << syscpu->tfreq() << std::endl;
        for (auto x : cpus)
        {
            std::cout << "CPU # " << x.hid << std::endl;
            std::cout << "+ State : " << (x.state ? "okay" : "failed") << std::endl;
            std::cout << "+ XLEN  : " << (int)x.xlen << std::endl;
            std::cout << "+ MMU   : SV" << (int)x.mmu << std::endl;
            std::cout << "+ Exts  : " << x.extension << std::endl;
        }
        std::cout << "====================" << std::endl;
    }

    k_cpuclock = syscpu->tfreq();
    if (!k_cpuclock)
        k_cpuclock = K_CONFIG_CPU_DEFAULT_CLOCK;

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
        // extern char _DRAM_BASE, _KERNEL_OFFSET;
        // sysmem->addReservedMem((size_t)&_DRAM_BASE, (size_t)&_KERNEL_OFFSET);
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
    int mmu_type = 0xFF;
    // find the smallest MMU type
    for (auto x : syscpu->CPUs())
    {
        if (x.mmu < mmu_type)
            mmu_type = x.mmu;
    }
    if (mmu_type == 0xFF || mmu_type == 32)
    { // Currently only support RV64
        std::cout << "[E] Invalid MMU config... Kernel Panic!" << std::endl;
        return K_EINVAL;
    }
    int mmu_variant = 0;
    auto vid = SBIF::Base::getVID();
    if (vid == 0x5B7) // T-head C906
        mmu_variant = RV64MMUBase::VARIANT_THEAD_C906;
    if (mmu_type == 39)
    {
        sysmmu = new SV39MMU(-1, mmu_variant);
        std::cout << "Using SV39 MMU\n";
    }
    else if (mmu_type == 48)
    {
        sysmmu = new SV48MMU(-1, mmu_variant);
        std::cout << "Using SV48 MMU\n";
    }
    else if (mmu_type == 57)
    {
        sysmmu = new SV57MMU(-1, mmu_variant);
        std::cout << "Using SV57 MMU\n";
    }
    else
    {
        std::cout << "[E] Unsupported MMU type... Kernel Panic!" << std::endl;
        return K_ENOTSUPP;
    }

    sysvmm = new VMemoryMgr(sysmmu);
    // Lower 4G: Direct mapping
    sysvmm->addMap(0xFFFFFFC000000000, 0, 2 * 1024 * 1048576ULL,
                   MMUBase::PROT_R | MMUBase::PROT_W | MMUBase::PROT_X | MMUBase::PROT_G | MMUBase::PROT_IO);
    sysvmm->addMap(0xFFFFFFC000000000 + 2 * 1024 * 1048576ULL, 2 * 1024 * 1048576ULL, 2 * 1024 * 1048576ULL,
                   MMUBase::PROT_R | MMUBase::PROT_W | MMUBase::PROT_X | MMUBase::PROT_G);
    rc = sysvmm->confirm();
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

    size_t boot_stack_size = K_CONFIG_KERNEL_STACK_SIZE;
    auto kstack = alignedMalloc<void>(boot_stack_size, 4096);
    sysvmm->addMap(-4*1024*1048576ULL - boot_stack_size, (uintptr_t)kstack & ~(0xFFFFFFC000000000), boot_stack_size,
                   MMUBase::PROT_R | MMUBase::PROT_W | MMUBase::PROT_X | MMUBase::PROT_G);
    rc = sysvmm->confirm();
    if (rc < 0)
    {
        std::cout << "[E] Failed to map global kernel stack: " << rc << std::endl;
        return K_EFAIL;
    }
    sysmmu->apply();

    *sys_stack_base = (void*)(-4*1024*1048576ULL) ;
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

    // Mount rootfs
    auto rootfs = sysroot->initrd();
    if (rootfs.empty())
    {
        std::cout << "[W] Rootfs not specified!" << std::endl;
    }
    else
    {
        std::cout << "Mounting rootfs: " << rootfs << std::endl;
        auto rc = VirtualFS::mount("/", rootfs.c_str(), 0, 0);
        if (rc < 0)
        {
            std::cout << "[E] Failed to mount rootfs: " << rc << std::endl;
            return rc;
        }
        std::cout << "[I] Rootfs mounted successfully!" << std::endl;
    }

    return 0;
}

int k_boot_harts(int boot_hartid)
{
    k_stage = K_BOOT_HARTS;
    extern char _start_hart;
    printf("> Booting harts... (Boot hart = %i)\n", boot_hartid);
    auto cpus = syscpu->CPUs();
    for (auto x : cpus)
    {
        int i = x.hid;
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
            auto t = csr_read(CSR_TIME);
            while (k_hart_state[i] < 2 && csr_read(CSR_TIME) - t < 3 * k_cpuclock)
                ; // wait for at most 3s
            if (k_hart_state[i] >= 2)
            {
                printf("OK!\n");
            }
            else
            {
                printf("Timeout!\n");
                k_hart_state[i] = 0; // reset to 0
            }
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

void k_before_cleanup()
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