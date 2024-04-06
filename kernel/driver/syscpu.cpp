#include "k_sysdev.h"

class GenericCPU : public SysCPU
{
  public:
    virtual int probe(const char *name, const char *compatible) override
    {
        if (name == std::string("cpus"))
            return DRV_CAP_THIS;
        if (name == std::string("cpu-map") || compatible == std::string("riscv"))
            return DRV_CAP_COVER; // also interrupt controller
        return DRV_CAP_NONE;
    }

    virtual long addDevice(const void *fdt, int node) override
    {
        auto name = fdt_get_name(fdt, node, nullptr);
        if (!name)
            return K_EINVAL;
        if (std::string(name) == "cpus")
        {
            int len = 0;
            auto tbase_freq = fdt_getprop(fdt, node, "timebase-frequency", &len);
            if (len > 0)
                timebase_freq = fdt32_to_cpu(*(fdt32_t *)tbase_freq);
            // printf("CPU Timebase frequency: %ld\n", timebase_freq);
            return 0xFFFF;
        }
        if (std::string(name) == "cpu-map")
        {
            // ignore for now
            return 0xFFF0;
        }

        // CPU defs
        cpu_t cpu;
        auto rc = fdt_get_node_addr_size(fdt, node, 0, &cpu.hid, nullptr);
        if (rc < 0)
        {
            printf("[E] Invalid CPU node ");
            return rc;
        }
        cpu.state = fdt_node_is_enabled(fdt, node);
        int len = 0;
        auto isa = fdt_getprop(fdt, node, "riscv,isa", &len);
        if (!isa || len < 5 || ((const char *)isa)[0] != 'r' || ((const char *)isa)[1] != 'v') // at least "rvxx_"
        {
            printf("[E] Invalid CPU ISA ");
            return K_EINVAL;
        }
        if (((const char *)isa)[2] == '3')
        {
            cpu.xlen = 32;
        }
        else if (((const char *)isa)[2] == '6')
        {
            cpu.xlen = 64;
        }
        else
        {
            printf("[E] Invalid CPU XLEN ");
            return K_EINVAL;
        }
        cpu.extension = std::string((const char *)isa, 4, len - 4);
        auto mmu = fdt_getprop(fdt, node, "mmu-type", &len);
        if (len < 0)
        {
            printf("[E] No MMU ");
            return K_EINVAL;
        }
        std::string mmutype((const char *)mmu, len);
        if (mmutype.find("sv32") != std::string::npos)
        {
            cpu.mmu = 32;
        }
        else if (mmutype.find("sv39") != std::string::npos)
        {
            cpu.mmu = 39;
        }
        else if (mmutype.find("sv48") != std::string::npos)
        {
            cpu.mmu = 48;
        }
        else if (mmutype.find("sv57") != std::string::npos)
        {
            cpu.mmu = 57;
        }
        else
        {
            printf("[E] Invalid CPU MMU type ");
            return K_EINVAL;
        }

        // interrupt controller not handled for now
        _cpus.push_back(cpu);

        return _hdlcount++;
    }

    virtual void removeDevice(long handler) override
    {
    }

  protected:
    long _hdlcount = 0;
};

static DRV_INSTALL_FUNC(K_PR_DEV_SYSCPU_END) void drv_register()
{
    static GenericCPU gcpu;
    DriverManager::addDriver(gcpu);
    printf("Driver GenericCPU installed\n");
}