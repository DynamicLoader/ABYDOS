
/**
 * @file uart8250.cpp
 * @author DynamicLoader
 * @brief UART8250 Driver
 * @version 0.1
 * @date 2024-03-10
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <cstdio>
#include <string>
#include "k_drvif.h"

static void drv_register();

class Drv_Uart8250 : public DriverBase
{
  public:
    Drv_Uart8250()
    {
        printf("Driver constructed\n");
    }
    
    bool probe(const char *devid) override
    {
        std::string id = devid;
        return (id == "ns16550" || id == "ns16550a" || id == "snps,dw-apb-uart");
    }

    long addDevice(const void *fdt) override
    {
        return ++hdl_count;
    }
    void removeDevice(long handler) override{

    }
    dev_type_t getDeviceType() override{
        return DEV_TYPE_CHAR;
    }

    // Remained devices should be closed in the destructor
    ~Drv_Uart8250()
    {
        printf("Driver destructed\n");
    }

  private:
    static int hdl_count;
    friend void drv_register();
};

int Drv_Uart8250::hdl_count = -1;


// We make a static instance of our driver, initialize and register it 
// Note that devices should be handled inside the class, not here
static DRV_BOOTUP_FUNC(200) void drv_register()
{
    static Drv_Uart8250 drv;
    drv.hdl_count = 0;
    k_add_driver(&drv);
    printf("Driver bootup\n");
}