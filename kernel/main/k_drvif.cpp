
#include <algorithm>
#include <vector>
#include <cstdio>

#include "k_drvif.h"
#include "k_defs.h"

#include "libfdt.h"

__attribute__((init_priority(K_PR_INIT_DRV_LIST))) std::vector<DriverBase *> DriverManager::_drvlist;

int DriverManager::probe(const void *fdt, int node)
{

    // memset(_depth, 0, sizeof(_depth));
    auto tret = _try(fdt, node);
    if (tret < 0)
    {
        printf("Error while probe node with offset: %i\n", node);
        return -1;
    }

    if (tret > 1)
    {
        printf("Driver has cover the node with offset: %i\n", node);
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
                probe(fdt, prev);
            prev = child;
        }
        probe(fdt, prev);
    }

    return 0;
}

int DriverManager::_try(const void *fdt, int node)
{
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

    printf("Finding driver for #%i %s [%s] ... ", node, node_name,compatible);
    fflush(stdout);

    int rc = 0;
    auto drv = std::find_if(_drvlist.begin(), _drvlist.end(), [&rc, node_name, compatible](DriverBase *drv) {
        return (rc = drv->probe(node_name, compatible)) > 0;
    });

    if (drv == _drvlist.end())
    {
        printf("Failed\n");
        return 0;
    }

    printf("Found! Installing... ");
    fflush(stdout);

    auto hdl = (*drv)->addDevice(fdt);
    if (hdl >= 0)
    {
        printf("OK, handler: %ld\n", hdl);
    }
    else
    {
        printf("Failed with %ld\n", hdl);
    }

    // dev handler to be handled

    return rc;
}
