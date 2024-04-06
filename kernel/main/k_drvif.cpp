
#include <algorithm>
#include <vector>
#include <cstdio>

#include "k_drvif.h"
#include "k_defs.h"

#include "libfdt.h"

__attribute__((init_priority(K_PR_INIT_DRV_LIST))) std::vector<DriverBase *> DriverManager::_drvlist;
__attribute__((init_priority(K_PR_INIT_DRV_LIST))) std::map<int, std::tuple<DriverBase *, int, long>>
    DriverManager::_devhdl;

int DriverManager::probe(const void *fdt, dev_type_t type, int node)
{

    // memset(_depth, 0, sizeof(_depth));
    auto tret = _try(fdt, node, type);
    if (tret < 0)
    {
        // printf("Error while probe node with offset: %i\n", node);
        return -1;
    }

    if (tret > 1)
    {
        // printf("Driver has cover the node with offset: %i\n", node);
        return 0;
    }

    // Recursive
    if (fdt_first_subnode(fdt, node) >= 0)
    {
        int child;
        int prev = -1;
        fdt_for_each_subnode(child, fdt, node)
        {
            if (prev >= 0)
                probe(fdt, type, prev);
            prev = child;
        }
        probe(fdt, type, prev);
    }

    return 0;
}

int DriverManager::_try(const void *fdt, int node, dev_type_t type)
{
    // Already installed
    auto fnd = _devhdl.find(node);
    if (fnd != _devhdl.end())
        return std::get<1>(fnd->second);

    const char *node_name = fdt_get_name(fdt, node, NULL);
    if (!node_name)
    {
        printf("Cannot get node name,skipped\n");
        return -1;
    }

    const struct fdt_property *prop;
    int size;
    prop = fdt_get_property(fdt, node, "compatible", &size);
    const char *compatible = prop ? prop->data : "";
    // printf("Finding driver for #%i %s [%s] ... ", node, node_name, compatible);
    fflush(stdout);

    int rc = 0;
    auto drv = std::find_if(_drvlist.begin(), _drvlist.end(), [&rc, node_name, compatible, type](DriverBase *drv) {
        auto dt = drv->getDeviceType();
        if ((type == DEV_TYPE_PERIP && dt > type) || (type == DEV_TYPE_NONE && dt > type) || type == dt)
            return (rc = drv->probe(node_name, compatible)) > 0;
        else
            return false;
    });

    if (drv == _drvlist.end())
    {
        // printf("Failed\n");
        return 0;
    }

    printf("Installing #%i %s [%s]... ",node,node_name,compatible);
    fflush(stdout);

    auto hdl = (*drv)->addDevice(fdt, node);
    if (hdl >= 0)
    {
        _devhdl.insert(std::make_pair(node, std::make_tuple(*drv, rc, hdl)));
        printf("OK, handler: %ld\n", hdl);
    }
    else
    {
        printf("Failed with %ld\n", hdl);
    }

    return rc;
}

long DriverManager::getDrvByPath(const void *fdt, const char *path, void **drv)
{
    auto rc = fdt_path_offset(fdt, path);
    if (rc < 0)
        return K_ENODEV;
    auto ret = _devhdl.find(rc);
    if (ret == _devhdl.end())
        return K_ENODEV;
    *drv = std::get<0>(ret->second);
    return std::get<2>(ret->second);
}
