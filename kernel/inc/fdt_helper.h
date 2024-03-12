
#ifndef __FDT_HELPER_H__
#define __FDT_HELPER_H__
#include "k_defs.h"
#include "k_types.h"
#include "libfdt.h"

#define FDT_MAX_PHANDLE_ARGS 16
struct fdt_phandle_args
{
    int node_offset;
    int args_count;
    u32 args[FDT_MAX_PHANDLE_ARGS];
};

#ifdef __cplusplus
extern "C"
{
#endif

    int fdt_parse_phandle_with_args(const void *fdt, int nodeoff, const char *prop, const char *cells_prop, int index,
                                    struct fdt_phandle_args *out_args);
    int fdt_get_node_addr_size(const void *fdt, int node, int index, uint64_t *addr, uint64_t *size);
    int fdt_get_node_addr_size_by_name(const void *fdt, int node, const char *name, uint64_t *addr, uint64_t *size);
    bool fdt_node_is_enabled(const void *fdt, int nodeoff);

#ifdef __cplusplus
}
#endif

#endif