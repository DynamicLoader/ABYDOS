/**
 * @file sysmem.cpp
 * @author DynamicLoader
 * @brief SysMem Driver
 * @version 0.1
 * @date 2024-03-11
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <cstdio>
#include <string>
#include "k_drvif.h"
#include "k_sysdev.h"

class GenericMem : public SysMem
{
  public:
    int probe(const char *name, const char *compatible) override
    {
        std::string id = name;
        if (id.find("memory") != 0 && id != "reserved-memory")
            return DRV_CAP_NONE;
        return DRV_CAP_COVER;
    }

    long addDevice(const void *fdt, int node) override
    {

        uint64_t address, size;
        if (!_fdt_rsv_mem_parsed)
        {
            // fdt command reserved memory
            int n = fdt_num_mem_rsv(fdt);
            for (int i = 0; i < n; ++i)
            {
                auto rc = fdt_get_mem_rsv(fdt, i, &address, &size);
                if (rc == 0)
                {
                    _mem_rsv.push_back(std::make_pair(address, size));
                    printf("(0x%lx - 0x%lx) ", address, address + size);
                }
            }
            _fdt_rsv_mem_parsed = true;

            // Some mark should be done?
        }

        auto name = fdt_get_name(fdt, node, NULL);
        if (name == std::string("reserved-memory"))
        {

            /* process reserved-memory node */
            auto nodeoffset = fdt_subnode_offset(fdt, 0, "reserved-memory");
            if (nodeoffset >= 0)
            {
                auto subnode = fdt_first_subnode(fdt, nodeoffset);
                while (subnode >= 0)
                {

                    if (fdt_get_node_addr_size(fdt, subnode, 0, &address, &size) >= 0)
                    { // each subnode should have only one addr and size
                        _mem_rsv.push_back(std::make_pair(address, size));
                        printf("(0x%lx - 0x%lx) ", address, address + size);
                    }
                    else
                    {
                        printf("Failed to get reserved memory in #%d\n", subnode);
                    }
                    subnode = fdt_next_subnode(fdt, subnode);
                }
            }
        }
        else
        { // memory node, could have multiple
            for (int i = 0; fdt_get_node_addr_size(fdt, node, i, &address, &size) >= 0; ++i)
            {
                _mem_avail.push_back(std::make_pair(address, size));
                printf("(0x%lx - 0x%lx) ", address, address + size);
            }
        }
        return 0; // Also singleton
    }

    void removeDevice(long handler) override
    {
    }

    void addReservedMem(uint64_t addr, uint64_t size) override
    {
        _mem_rsv.push_back(std::make_pair(addr, size));
    }

  private:
    bool _fdt_rsv_mem_parsed = false;
};

// We make a static instance of our driver, initialize and register it
// Note that devices should be handled inside the class, not here
static DRV_INSTALL_FUNC(K_PR_DEV_SYSMEM_END) void drv_register()
{
    static GenericMem drv;
    DriverManager::addDriver(drv);
    printf("Driver GenericMem installed\n");
}