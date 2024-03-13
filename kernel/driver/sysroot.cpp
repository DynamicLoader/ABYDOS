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

#include "k_sysdev.h"

class GenericRoot : public SysRoot
{
  public:
    int probe(const char *name, const char *compatible) override
    {
        std::string id = name;
        if (id == "")
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
                DriverManager::markInstalled(fdt, rc, 0, this,DRV_CAP_THIS); // mark as installed

                return 0; // singleton device
            }
        }

        return K_ENODEV;
    }

    void removeDevice(long handler) override
    {
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