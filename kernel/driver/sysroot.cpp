/**
 * @file sysroot.cpp
 * @author DynamicLoader
 * @brief SYS Root Driver, base board and "chosen device"
 * @version 0.1
 * @date 2024-03-11
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <string_view>
#include <stdexcept>
#include "k_sysdev.h"

class GenericRoot : public SysRoot
{
  public:
    int probe(const char *name, const char *compatible) override
    {
        using namespace std::string_view_literals;
        if (name == ""sv)
            return DRV_CAP_THIS;
        return DRV_CAP_NONE;
    }

    long addDevice(const void *fdt, int node) override
    {
        int len;
        if (node == 0) // root node
        {
            auto prop = fdt_get_property(fdt, node, "compatible", &len);
            if (!prop)
                _compatible = "Generic";
            else
                _compatible = std::string(prop->data);
            prop = fdt_get_property(fdt, node, "model", &len);
            if (!prop)
                _model = "Generic";
            else
                _model = std::string(prop->data);

            auto rc = fdt_path_offset(fdt, "/chosen"); // property of chosen node may diff on other platform
            if (rc >= 0)
            {
                auto prop = fdt_get_property(fdt, rc, "stdout-path", &len);
                if (prop)
                    _stdout_path = std::string(prop->data);

                prop = fdt_get_property(fdt, rc, "linux,initrd-start", &len); // Just compatible with Linux
                if (prop)
                {
                    size_t t = 0, e = 0;
                    if (len == 4)
                    {
                        t = fdt32_to_cpu(*((fdt32_t *)(prop->data)));
                    }
                    if (len == 8)
                    {
                        t = fdt64_to_cpu(*((fdt64_t *)(prop->data)));
                    }

                    prop = fdt_get_property(fdt, rc, "linux,initrd-end", &len); // Just compatible with Linux
                    if (prop)
                    {
                        if (len == 4)
                        {
                            e = fdt32_to_cpu(*((fdt32_t *)(prop->data)));
                        }
                        if (len == 8)
                        {
                            e = fdt64_to_cpu(*((fdt64_t *)(prop->data)));
                        }
                    }

                    if (t && e && e > t)
                    {
                        t |= 0xFFFFFFC000000000; // Conv to sysmem address
                        e |= 0xFFFFFFC000000000; // Conv to sysmem address
                        _initrd_path = std::to_string(t) + "," + std::to_string(e - t);
                    }
                }

                prop = fdt_get_property(fdt, rc, "bootargs", &len);
                if (prop)
                {
                    _bootargs = std::string(prop->data);
                    // Process bootargs
                    auto st = _bootargs.find("-use-scheduler-");
                    if (st != std::string::npos)
                    {
                        st += 15;
                        auto end = _bootargs.find(' ', st);
                        if (end > st)
                        {
                            auto rc = DriverManager::getDriverByProbe(("scheduler-" + _bootargs.substr(st,end-st)).c_str(), "sys,scheduler");
                            if (rc)
                            {
                                _scheduler = (SysScheduler *)rc;
                                printf("[Using scheduler = %s] ", _bootargs.substr(st,end).c_str());
                            }
                        }
                    }
                }

                // Fallbacks
                if (!_scheduler)
                {
                    auto rc = DriverManager::getDriverByProbe(K_CONFIG_DEFAULT_SCHEDULER, "sys,scheduler");
                    if (rc)
                    {
                        _scheduler = (SysScheduler *)rc;
                        printf("[Using default scheduler = " K_CONFIG_DEFAULT_SCHEDULER "] ");
                    }
                    else
                    {
                        printf("[E] Default scheduler not found! ");
                        return K_EINVAL;
                    }
                }
                DriverManager::markInstalled(fdt, rc, 0, this, DRV_CAP_THIS); // mark as installed

                return 0; // singleton device
            }
        }

        return K_ENODEV;
    }

    void removeDevice(long handler) override
    {
        throw std::runtime_error("Cannot remove root device");
    }
};

// We make a static instance of our driver, initialize and register it
// Note that devices should be handled inside the class, not here
static DRV_INSTALL_FUNC(K_PR_DRV_SYSROOT_END) void drv_register()
{
    static GenericRoot drv;
    DriverManager::addDriver(drv);
    printf("Driver GenericRoot installed\n");
}