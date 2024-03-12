

#include "fdt_helper.h"


int fdt_parse_phandle_with_args(const void *fdt, int nodeoff,
				const char *prop, const char *cells_prop,
				int index, struct fdt_phandle_args *out_args)
{
	u32 i, pcells;
	int len, pnodeoff;
	const fdt32_t *list, *list_end, *val;

	if (!fdt || (nodeoff < 0) || !prop || !cells_prop || !out_args)
		return K_EINVAL;

	list = fdt_getprop(fdt, nodeoff, prop, &len);
	if (!list)
		return K_ENOENT;
	list_end = list + (len / sizeof(*list));

	while (list < list_end) {
		pnodeoff = fdt_node_offset_by_phandle(fdt,
						fdt32_to_cpu(*list));
		if (pnodeoff < 0)
			return pnodeoff;
		list++;

		val = fdt_getprop(fdt, pnodeoff, cells_prop, &len);
		if (!val)
			return K_ENOENT;
		pcells = fdt32_to_cpu(*val);
		if (FDT_MAX_PHANDLE_ARGS < pcells)
			return K_EINVAL;
		if (list + pcells > list_end)
			return K_ENOENT;

		if (index > 0) {
			list += pcells;
			index--;
		} else {
			out_args->node_offset = pnodeoff;
			out_args->args_count = pcells;
			for (i = 0; i < pcells; i++)
				out_args->args[i] = fdt32_to_cpu(list[i]);
			return 0;
		}
	}

	return K_ENOENT;
}

static int fdt_translate_address(const void *fdt, uint64_t reg, int parent,
				 uint64_t *addr)
{
	int i, rlen;
	int cell_addr, cell_size;
	const fdt32_t *ranges;
	uint64_t offset, caddr = 0, paddr = 0, rsize = 0;

	cell_addr = fdt_address_cells(fdt, parent);
	if (cell_addr < 1)
		return K_ENODEV;

	cell_size = fdt_size_cells(fdt, parent);
	if (cell_size < 0)
		return K_ENODEV;

	ranges = fdt_getprop(fdt, parent, "ranges", &rlen);
	if (ranges && rlen > 0) {
		for (i = 0; i < cell_addr; i++)
			caddr = (caddr << 32) | fdt32_to_cpu(*ranges++);
		for (i = 0; i < cell_addr; i++)
			paddr = (paddr << 32) | fdt32_to_cpu(*ranges++);
		for (i = 0; i < cell_size; i++)
			rsize = (rsize << 32) | fdt32_to_cpu(*ranges++);
		if (reg < caddr || caddr >= (reg + rsize )) {
			// K_printf("invalid address translation\n");
			return K_ENODEV;
		}
		offset = reg - caddr;
		*addr = paddr + offset;
	} else {
		/* No translation required */
		*addr = reg;
	}

	return 0;
}

int fdt_get_node_addr_size(const void *fdt, int node, int index,
			   uint64_t *addr, uint64_t *size)
{
	int parent, len, i, rc;
	int cell_addr, cell_size;
	const fdt32_t *prop_addr, *prop_size;
	uint64_t temp = 0;

	if (!fdt || node < 0 || index < 0)
		return K_EINVAL;

	parent = fdt_parent_offset(fdt, node);
	if (parent < 0)
		return parent;
	cell_addr = fdt_address_cells(fdt, parent);
	if (cell_addr < 1)
		return K_ENODEV;

	cell_size = fdt_size_cells(fdt, parent);
	if (cell_size < 0)
		return K_ENODEV;

	prop_addr = fdt_getprop(fdt, node, "reg", &len);
	if (!prop_addr)
		return K_ENODEV;

	if ((len / sizeof(u32)) <= (index * (cell_addr + cell_size)))
		return K_EINVAL;

	prop_addr = prop_addr + (index * (cell_addr + cell_size));
	prop_size = prop_addr + cell_addr;

	if (addr) {
		for (i = 0; i < cell_addr; i++)
			temp = (temp << 32) | fdt32_to_cpu(*prop_addr++);
		do {
			if (parent < 0)
				break;
			rc  = fdt_translate_address(fdt, temp, parent, addr);
			if (rc)
				break;
			parent = fdt_parent_offset(fdt, parent);
			temp = *addr;
		} while (1);
	}
	temp = 0;

	if (size) {
		for (i = 0; i < cell_size; i++)
			temp = (temp << 32) | fdt32_to_cpu(*prop_size++);
		*size = temp;
	}

	return 0;
}

int fdt_get_node_addr_size_by_name(const void *fdt, int node, const char *name,
				   uint64_t *addr, uint64_t *size)
{
	int i, j, count;
	const char *val;
	const char *regname;

	if (!fdt || node < 0 || !name)
		return K_EINVAL;

	val = fdt_getprop(fdt, node, "reg-names", &count);
	if (!val)
		return K_ENODEV;

	for (i = 0, j = 0; i < count; i++, j++) {
		regname = val + i;

		if (strcmp(name, regname) == 0)
			return fdt_get_node_addr_size(fdt, node, j, addr, size);

		i += strlen(regname);
	}

	return K_ENODEV;
}

bool fdt_node_is_enabled(const void *fdt, int nodeoff)
{
	int len;
	const void *prop;

	prop = fdt_getprop(fdt, nodeoff, "status", &len);
	if (!prop)
		return true;

	if (!strncmp(prop, "okay", strlen("okay")))
		return true;

	if (!strncmp(prop, "ok", strlen("ok")))
		return true;

	return false;
}