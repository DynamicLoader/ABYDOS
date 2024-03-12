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
    // int probe(const char *name, const char *compatible) override;
    // long addDevice(const void *fdt, int node) override;
    // void removeDevice(long handler) override;
    // dev_type_t getDeviceType() override;

    __K_PROP_EXPORT__(compatible, _compatible)
    __K_PROP_EXPORT__(model, _model)
    __K_PROP_EXPORT__(stdout_path, _stdout_path)

  protected:
    std::string _compatible;
    std::string _model;
    std::string _stdout_path;
};

#endif