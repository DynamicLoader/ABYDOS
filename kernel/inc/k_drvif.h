#ifndef __K_DRVIF_H__
#define __K_DRVIF_H__

#define DRV_BOOTUP_FUNC(V) __attribute__((constructor(V)))

typedef enum
{
    DEV_TYPE_CHAR = 1,
    DEV_TYPE_BLOCK,
} dev_type_t;


#ifdef __cplusplus

#include <vector>

class DriverBase
{
  public:
    virtual bool probe(const char *devid) = 0;
    virtual long addDevice(const void *fdt) = 0; // returns handler
    virtual void removeDevice(long handler) = 0;  // unregister device by handler
    virtual dev_type_t getDeviceType() = 0;

    // virtual ~DriverBase() = default;
};

void k_add_driver(DriverBase *drv);

#endif

#endif