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

  protected:
    std::string _compatible;
    std::string _model;
    std::string _stdout_path;
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

    virtual void addReservedMem(uint64_t addr,uint64_t size) = 0;

    __K_PROP_EXPORT__(reservedMem, _mem_rsv);
    __K_PROP_EXPORT__(availableMem, _mem_avail);

    protected:
        std::vector<std::pair<size_t,size_t>> _mem_rsv;
        std::vector<std::pair<size_t,size_t>> _mem_avail;
};

#undef __K_PROP_EXPORT__

#endif