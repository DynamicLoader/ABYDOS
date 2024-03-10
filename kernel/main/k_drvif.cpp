
#include <vector>
#include <cstdio>

#include "k_drvif.h"

std::vector<DriverBase *> *k_drv_list;


// These two function is to be called before or after global con/destructors, so never use std::cout or so on
extern "C"{

void k_drv_init()
{
    printf("k_drv_init\n");
    k_drv_list = new std::vector<DriverBase *>();
}

void k_drv_deinit()
{
    printf("k_drv_deinit\n");
    if (k_drv_list != nullptr)
        delete k_drv_list;
}

} // extern "C"

void k_add_driver(DriverBase *drv)
{
    if (drv != nullptr)
        k_drv_list->push_back(drv);
}

