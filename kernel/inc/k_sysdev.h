#ifndef __K_SYSDEV_H__
#define __K_SYSDEV_H__

#include <cstdio>
#include <string>
#include "k_drvif.h"

#define __K_PROP_EXPORT__(name, pri)                                                                                   \
    const auto &name() const                                                                                           \
    {                                                                                                                  \
        return pri;                                                                                                    \
    }

class SysScheduler : public DriverBase
{
  public:
    struct task_t
    {
        int priority;
        uintptr_t start;
    };

    dev_type_t getDeviceType() override
    {
        return DEV_TYPE_SYS;
    }

    virtual uint32_t addTask(long hdl, const task_t &) = 0;
    virtual int removeTask(long hdl, uint32_t) = 0;

  protected:
};

class SysRoot : public DriverBase
{
  public:
    virtual dev_type_t getDeviceType() override
    {
        return DEV_TYPE_SYS;
    }

    __K_PROP_EXPORT__(compatible, _compatible)
    __K_PROP_EXPORT__(model, _model)
    __K_PROP_EXPORT__(stdout_path, _stdout_path)
    __K_PROP_EXPORT__(bootargs, _bootargs)
    __K_PROP_EXPORT__(scheduler, _scheduler)
    __K_PROP_EXPORT__(initrd, _initrd_path)

  protected:
    std::string _compatible;
    std::string _model;
    std::string _stdout_path;
    std::string _bootargs;
    std::string _initrd_path;
    SysScheduler *_scheduler = nullptr;
};

// If custom SysMem class is provided in platform, SysRoot should be implemented as well,
// with probe() to remove the platform-specified SysMem instance from DriverManager when not suitable.
class SysMem : public DriverBase
{
  public:
    dev_type_t getDeviceType() override
    {
        return DEV_TYPE_SYS;
    }

    virtual void addReservedMem(uint64_t addr, uint64_t size) = 0;

    __K_PROP_EXPORT__(reservedMem, _mem_rsv)
    __K_PROP_EXPORT__(availableMem, _mem_avail)

  protected:
    std::vector<std::pair<size_t, size_t>> _mem_rsv;
    std::vector<std::pair<size_t, size_t>> _mem_avail;
};

class SysCPU : public DriverBase
{
  public:
    struct cpu_t
    {
        uint64_t hid;
        bool state;
        uint8_t xlen;
        uint8_t mmu;
        std::string extension;
    };

    dev_type_t getDeviceType() override
    {
        return DEV_TYPE_SYS;
    }

    __K_PROP_EXPORT__(tfreq, timebase_freq)
    __K_PROP_EXPORT__(CPUs, _cpus)

  protected:
    unsigned long timebase_freq = 0; // 0 by default, which means no timebase

    std::vector<cpu_t> _cpus;
};

#undef __K_PROP_EXPORT__

#endif