#include "k_main.h"
#include "k_defs.h"

struct umode_global_ctx_t
{
    unsigned long ra, sp, gp, tp;
    unsigned long t[7];
    unsigned long a[8];
    unsigned long s[12];
    unsigned long pc;
};

// https://sourceware.org/binutils/docs/as/RISC_002dV_002dDirectives.html
template <uint8_t flen> struct umode_float_ctx_t
{
    uint32_t f[32 * (flen >> 5)];
    uint32_t fcsr;

    // clang-format off

#define _fs_ops_l(fsop) \
    fsop " f8, %0 \n" \
    fsop " f1, %1 \n" \
    fsop " f2, %2 \n" \
    fsop " f3, %3 \n" \
    fsop " f4, %4 \n" \
    fsop " f5, %5 \n" \
    fsop " f6, %6 \n" \
    fsop " f7, %7 \n" \
    fsop " f8, %8 \n" \
    fsop " f9, %9 \n" \
    fsop " f10, %10 \n" \
    fsop " f11, %11 \n" \
    fsop " f12, %12 \n" \
    fsop " f13, %13 \n" \
    fsop " f14, %14 \n" \
    fsop " f15, %15 \n" \

#define _fs_ops_h(fsop) \
    fsop " f16, %0 \n" \
    fsop " f17, %1 \n" \
    fsop " f18, %2 \n" \
    fsop " f19, %3 \n" \
    fsop " f20, %4 \n" \
    fsop " f21, %5 \n" \
    fsop " f22, %6 \n" \
    fsop " f23, %7 \n" \
    fsop " f24, %8 \n" \
    fsop " f25, %9 \n" \
    fsop " f26, %10 \n" \
    fsop " f27, %11 \n" \
    fsop " f28, %12 \n" \
    fsop " f29, %13 \n" \
    fsop " f30, %14 \n" \
    fsop " f31, %15 \n"

#define _fs_constraint_l(constraint,array,ljust) \
    constraint (array [0 << ljust]), \
    constraint (array [1 << ljust]), \
    constraint (array [2 << ljust]), \
    constraint (array [3 << ljust]), \
    constraint (array [4 << ljust]), \
    constraint (array [5 << ljust]), \
    constraint (array [6 << ljust]), \
    constraint (array [7 << ljust]), \
    constraint (array [8 << ljust]), \
    constraint (array [9 << ljust]), \
    constraint (array [10 << ljust]), \
    constraint (array [11 << ljust]), \
    constraint (array [12 << ljust]), \
    constraint (array [13 << ljust]), \
    constraint (array [14 << ljust]), \
    constraint (array [15 << ljust]) \

#define _fs_constraint_h(constraint,array,ljust) \
    constraint (array [16 << ljust]), \
    constraint (array [17 << ljust]), \
    constraint (array [18 << ljust]), \
    constraint (array [19 << ljust]), \
    constraint (array [20 << ljust]), \
    constraint (array [21 << ljust]), \
    constraint (array [22 << ljust]), \
    constraint (array [23 << ljust]), \
    constraint (array [24 << ljust]), \
    constraint (array [25 << ljust]), \
    constraint (array [26 << ljust]), \
    constraint (array [27 << ljust]), \
    constraint (array [28 << ljust]), \
    constraint (array [29 << ljust]), \
    constraint (array [30 << ljust]), \
    constraint (array [31 << ljust])

#define _fs_ext_enable(ext) \
    asm volatile( \
        ".option push \n" \
        ".option arch, +" ext "\n" \
    )

#define _fs_ext_disable() \
    asm volatile( \
        ".option pop" \
    )
    // clang-format on

    // must be executed after main context is saved
    void save()
    {

        if constexpr (flen == 32)
        {
            _fs_ext_enable("f");
            asm volatile(_fs_ops_l("fsw") : _fs_constraint_l("=m", f, 0));
            asm volatile(_fs_ops_h("fsw") : _fs_constraint_h("=m", f, 0));
            fcsr = csr_read(CSR_FCSR);
            _fs_ext_disable();
        }
        else if constexpr (flen == 64)
        {
            _fs_ext_enable("d");
            asm volatile(_fs_ops_l("fsd") : _fs_constraint_l("=m", f, 1));
            asm volatile(_fs_ops_h("fsd") : _fs_constraint_h("=m", f, 1));
            fcsr = csr_read(CSR_FCSR);
            _fs_ext_disable();
        }
        else if constexpr (flen == 128)
        {
            _fs_ext_enable("q");
            asm volatile(_fs_ops_l("fsq") : _fs_constraint_l("=m", f, 2));
            asm volatile(_fs_ops_h("fsq") : _fs_constraint_h("=m", f, 2));
            fcsr = csr_read(CSR_FCSR);
            _fs_ext_disable();
        }
        else
        {
            // Do nothing
        }
    }

    // must be executed before main context is restored
    void restore()
    {
        if constexpr (flen == 32)
        {
            _fs_ext_enable("f");
            asm volatile(_fs_ops_l("flw")::_fs_constraint_l("m", f, 0));
            asm volatile(_fs_ops_h("flw")::_fs_constraint_h("m", f, 0));
            csr_write(CSR_FCSR, fcsr);
            _fs_ext_disable();
        }
        else if constexpr (flen == 64)
        {
            _fs_ext_enable("d");
            asm volatile(_fs_ops_l("fld")::_fs_constraint_l("m", f, 1));
            asm volatile(_fs_ops_h("fld")::_fs_constraint_h("m", f, 2));
            csr_write(CSR_FCSR, fcsr);
            _fs_ext_disable();
        }
        else if constexpr (flen == 128)
        {
            _fs_ext_enable("q");
            asm volatile(_fs_ops_l("flq")::_fs_constraint_l("m", f, 2));
            asm volatile(_fs_ops_h("flq")::_fs_constraint_h("m", f, 2));
            csr_write(CSR_FCSR, fcsr);
            _fs_ext_disable();
        }
        else
        {
            // Do nothing
        }
    }

#undef _fs_ops
#undef _fs_constraint
#undef _fs_ext_enable
#undef _fs_ext_disable
};

umode_float_ctx_t<32> umode_single_float_ctx;

__attribute__((constructor())) void __()
{
    umode_single_float_ctx.save();
    umode_single_float_ctx.restore();
}