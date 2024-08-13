#ifndef __K_DRVIF_H__
#define __K_DRVIF_H__

#define DRV_INSTALL_FUNC(V) __attribute__((constructor(V)))
#define DRV_CAP_NONE 0
#define DRV_CAP_THIS 1
#define DRV_CAP_COVER 2

typedef enum
{
    DEV_TYPE_NONE = 0,
    DEV_TYPE_SYS,
    DEV_TYPE_PERIP = 0x80,
    DEV_TYPE_CHAR,
    DEV_TYPE_BLOCK,
} dev_type_t;

#ifdef __cplusplus

#include <vector>
#include <map>

extern "C"
{
#include "fdt_helper.h"
}

class DriverBase
{
  public:
    // Returns 1 if driver can handle this device, or 2 for cover-all driver (will stop probing for sub nodes)
    virtual int probe(const char *name, const char *compatible) = 0;
    virtual long addDevice(const void *fdt, int node) = 0; // returns handler
    virtual void removeDevice(long handler) = 0;           // unregister device by handler
    virtual dev_type_t getDeviceType() = 0;

    virtual ~DriverBase() = default;
};

class DriverChar : public DriverBase
{
  public:
    virtual int open(long handler) = 0;
    virtual int close(long handler) = 0;
    virtual int read(long handler, void *buf, int len) = 0;
    virtual int write(long handler, const void *buf, int len) = 0;
    virtual int ioctl(long handler, int cmd, void *arg) = 0;

    virtual dev_type_t getDeviceType() override
    {
        return DEV_TYPE_CHAR;
    }
};

class DriverBlock : public DriverBase
{
  public:
    virtual int read(long handler, void *buf, int len, int offset) = 0;
    virtual int write(long handler, const void *buf, int len, int offset) = 0;
    virtual int ioctl(long handler, int cmd, void *arg) = 0;

    virtual dev_type_t getDeviceType() override
    {
        return DEV_TYPE_BLOCK;
    }
};

class DriverManager
{
  public:
    static void addDriver(DriverBase &drv)
    {
        _drvlist.push_back(&drv);
    }

    static int markInstalled(const void *fdt, int node, long handler, DriverBase *drv, int drv_cap)
    {
        auto rc = fdt_get_name(fdt, node, NULL);
        if (!rc)
            return -1;
        _devhdl.insert(std::make_pair(node, std::make_tuple(drv, drv_cap, handler)));
        extern int printf(const char *fmt, ...);
        printf("(#%i [%s] => %ld) ", node, rc, handler);
        return 0;
    }

    // The function would not delete the device, so the handler is still valid
    static void removeDriver(DriverBase &drv)
    {
        for (auto it = _drvlist.begin(); it != _drvlist.end(); ++it)
        {
            if (*it == &drv)
            {
                _drvlist.erase(it);
                break;
            }
        }
    }

    static DriverBase *getDriverByProbe(const char *name, const char *compatible)
    {
        for (auto &drv : _drvlist)
        {
            if (drv->probe(name, compatible) > 0)
                return drv;
        }
        return nullptr;
    }

    // static long find
    static int probe(const void *fdt, dev_type_t type = DEV_TYPE_PERIP, int node = 0);
    static long getDrvByPath(const void *fdt, const char *path, void **drv);

  private:
    static std::vector<DriverBase *> _drvlist;
    static std::map<int, std::tuple<DriverBase *, int, long>> _devhdl; // node, <drv,rc,hdl>
    static int _try(const void *fdt, int node, dev_type_t type);
};

#else

// For C only driver

#endif

#endif