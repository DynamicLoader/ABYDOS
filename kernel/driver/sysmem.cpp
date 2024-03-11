/**
 * @file sysmem.cpp
 * @author DynamicLoader
 * @brief UART8250 Driver
 * @version 0.1
 * @date 2024-03-11
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <cstdio>
#include <string>
#include "k_drvif.h"

static void drv_register();

class SysMem : public DriverBase
{
  public:

    int probe(const char *name, const char *compatible) override
    {
        std::string id = name;
        if (id.find("memory") == std::string::npos && id != "reserved-memory")
            return DRV_CAP_NONE;
        return DRV_CAP_THIS;
    }

    long addDevice(const void *fdt) override
    {
        return ++hdl_count;
    }
    void removeDevice(long handler) override
    {
    }
    dev_type_t getDeviceType() override
    {
        return DEV_TYPE_SYS;
    }

  private:
    static int hdl_count;
    friend void drv_register();
};

int SysMem::hdl_count = -1;

// We make a static instance of our driver, initialize and register it
// Note that devices should be handled inside the class, not here
static DRV_INSTALL_FUNC(210) void drv_register()
{
    static SysMem drv;
    drv.hdl_count = 0;
    DriverManager::addDriver(drv);
    printf("Driver SysMem installed\n");
}