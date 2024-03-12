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
        return DRV_CAP_COVER;
    }

    long addDevice(const void *fdt, int node) override
    {
        uint64_t address, size;
        int n = fdt_num_mem_rsv(fdt);
        for (int i = 0; i < n; ++i)
        {
            auto rc = fdt_get_mem_rsv(fdt, i, &address, &size);
            if (rc == 0)
                printf("Got reserved memory: 0x%lx - 0x%lx\n", address, address + size);
        }

        /* process reserved-memory */
        auto nodeoffset = fdt_subnode_offset(fdt, 0, "reserved-memory");
        if (nodeoffset >= 0)
        {
            auto subnode = fdt_first_subnode(fdt, nodeoffset);
            while (subnode >= 0)
            {
                // Todo: process reserved-memory
                subnode = fdt_next_subnode(fdt, subnode);
            }
        }
        return 0; // Also singelton
    }

    void removeDevice(long handler) override
    {
    }
    dev_type_t getDeviceType() override
    {
        return DEV_TYPE_SYS;
    }

  private:
    friend void drv_register();
};

// We make a static instance of our driver, initialize and register it
// Note that devices should be handled inside the class, not here
static DRV_INSTALL_FUNC(K_PR_DEV_SYSMEM_END) void drv_register()
{
    static SysMem drv;
    DriverManager::addDriver(drv);
    printf("Driver SysMem installed\n");
}