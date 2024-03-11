#ifndef __K_DRVIF_H__
#define __K_DRVIF_H__

#define DRV_INSTALL_FUNC(V) __attribute__((constructor(V)))
#define DRV_CAP_NONE 0
#define DRV_CAP_THIS 1
#define DRV_CAP_COVER 2

typedef enum
{
    DEV_TYPE_SYS = 1,
    DEV_TYPE_CHAR = 0x80,
    DEV_TYPE_BLOCK,
} dev_type_t;

#ifdef __cplusplus

#include <vector>

class DriverBase
{
  public:
  // Returns 1 if driver can handle this device, or 2 for cover-all driver (will stop probing for sub nodes)
    virtual int probe(const char *name, const char *compatible) = 0;
    virtual long addDevice(const void *fdt) = 0; // returns handler
    virtual void removeDevice(long handler) = 0; // unregister device by handler
    virtual dev_type_t getDeviceType() = 0;

    virtual ~DriverBase() = default;
};

// void k_add_driver(DriverBase *drv);

class DriverManager
{
  public:
    static void addDriver(DriverBase &drv)
    {
        _drvlist.push_back(&drv);
    }
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

  protected:
    static int probe(const void *fdt,int node = 0);
    friend int k_main(int argc, const char *argv[]);

  private:
    static std::vector<DriverBase *> _drvlist;
    static int _try(const void *fdt, int node);
};

#else

// For C only driver

#endif

#endif