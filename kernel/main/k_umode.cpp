#include "k_main.h"
#include "k_defs.h"

// clang-format off

#define _umode_ls_ops(op,regprefix) \
    op " " regprefix "%8, %0 \n" \
    op " " regprefix "%9, %1 \n" \
    op " " regprefix "%10, %2 \n" \
    op " " regprefix "%11, %3 \n" \
    op " " regprefix "%12, %4 \n" \
    op " " regprefix "%13, %5 \n" \
    op " " regprefix "%14, %6 \n" \
    op " " regprefix "%15, %7 \n"

#define _umode_ls_cons_i(op,start)\
    op (start+0), \
    op (start+1), \
    op (start+2), \
    op (start+3), \
    op (start+4), \
    op (start+5), \
    op (start+6), \
    op (start+7)

#define _umode_ls_cons_arr(op, arr, lj,start) \
    op (arr[(start + 0) << lj]), \
    op (arr[(start + 1) << lj]), \
    op (arr[(start + 2) << lj]), \
    op (arr[(start + 3) << lj]), \
    op (arr[(start + 4) << lj]), \
    op (arr[(start + 5) << lj]), \
    op (arr[(start + 6) << lj]), \
    op (arr[(start + 7) << lj])

// https://sourceware.org/binutils/docs/as/RISC_002dV_002dDirectives.html
#define _umode_ext_enable(ext) \
    asm volatile( \
        ".option push \n" \
        ".option arch, +" ext "\n" \
    )

#define _umode_ext_disable() \
    asm volatile( \
        ".option pop" \
    )
// clang-format on

struct umode_global_ctx_t
{
    unsigned long ra, sp, gp, tp;
    unsigned long t[7];
    unsigned long a[8];
    unsigned long s[12];
    unsigned long pc;
};

template <uint8_t flen> struct umode_float_ctx_t
{
    uint32_t f[32 * (flen >> 5)];
    uint32_t fcsr;

    // must be executed after main context is saved
    void save()
    {

        if constexpr (flen == 32)
        {
            _umode_ext_enable("f");
            asm volatile(_umode_ls_ops("fsw", "f") : _umode_ls_cons_arr("=m", f, 0, 0) : _umode_ls_cons_i("i", 0));
            asm volatile(_umode_ls_ops("fsw", "f") : _umode_ls_cons_arr("=m", f, 0, 8) : _umode_ls_cons_i("i", 8));
            asm volatile(_umode_ls_ops("fsw", "f") : _umode_ls_cons_arr("=m", f, 0, 16) : _umode_ls_cons_i("i", 16));
            asm volatile(_umode_ls_ops("fsw", "f") : _umode_ls_cons_arr("=m", f, 0, 24) : _umode_ls_cons_i("i", 24));
            fcsr = csr_read(CSR_FCSR);
            _umode_ext_disable();
        }
        else if constexpr (flen == 64)
        {
            _umode_ext_enable("d");
            asm volatile(_umode_ls_ops("fsd", "f") : _umode_ls_cons_arr("=m", f, 0, 0) : _umode_ls_cons_i("i", 0));
            asm volatile(_umode_ls_ops("fsd", "f") : _umode_ls_cons_arr("=m", f, 0, 8) : _umode_ls_cons_i("i", 8));
            asm volatile(_umode_ls_ops("fsd", "f") : _umode_ls_cons_arr("=m", f, 0, 16) : _umode_ls_cons_i("i", 16));
            asm volatile(_umode_ls_ops("fsd", "f") : _umode_ls_cons_arr("=m", f, 0, 24) : _umode_ls_cons_i("i", 24));
            fcsr = csr_read(CSR_FCSR);
            _umode_ext_disable();
        }
        else if constexpr (flen == 128)
        {
            _umode_ext_enable("q");
            asm volatile(_umode_ls_ops("fsq", "f") : _umode_ls_cons_arr("=m", f, 0, 0) : _umode_ls_cons_i("i", 0));
            asm volatile(_umode_ls_ops("fsq", "f") : _umode_ls_cons_arr("=m", f, 0, 8) : _umode_ls_cons_i("i", 8));
            asm volatile(_umode_ls_ops("fsq", "f") : _umode_ls_cons_arr("=m", f, 0, 16) : _umode_ls_cons_i("i", 16));
            asm volatile(_umode_ls_ops("fsq", "f") : _umode_ls_cons_arr("=m", f, 0, 24) : _umode_ls_cons_i("i", 24));
            fcsr = csr_read(CSR_FCSR);
            _umode_ext_disable();
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
            _umode_ext_enable("f");
            asm volatile(_umode_ls_ops("flw", "f") : : _umode_ls_cons_arr("m", f, 0, 0), _umode_ls_cons_i("i", 0));
            asm volatile(_umode_ls_ops("flw", "f") : : _umode_ls_cons_arr("m", f, 0, 8), _umode_ls_cons_i("i", 8));
            asm volatile(_umode_ls_ops("flw", "f") : : _umode_ls_cons_arr("m", f, 0, 16), _umode_ls_cons_i("i", 16));
            asm volatile(_umode_ls_ops("flw", "f") : : _umode_ls_cons_arr("m", f, 0, 24), _umode_ls_cons_i("i", 24));
            csr_write(CSR_FCSR, fcsr);
            _umode_ext_disable();
        }
        else if constexpr (flen == 64)
        {
            _umode_ext_enable("d");
            asm volatile(_umode_ls_ops("fld", "f") : : _umode_ls_cons_arr("m", f, 0, 0), _umode_ls_cons_i("i", 0));
            asm volatile(_umode_ls_ops("fld", "f") : : _umode_ls_cons_arr("m", f, 0, 8), _umode_ls_cons_i("i", 8));
            asm volatile(_umode_ls_ops("fld", "f") : : _umode_ls_cons_arr("m", f, 0, 16), _umode_ls_cons_i("i", 16));
            asm volatile(_umode_ls_ops("fld", "f") : : _umode_ls_cons_arr("m", f, 0, 24), _umode_ls_cons_i("i", 24));
            csr_write(CSR_FCSR, fcsr);
            _umode_ext_disable();
        }
        else if constexpr (flen == 128)
        {
            _umode_ext_enable("q");
            asm volatile(_umode_ls_ops("flq", "f") : : _umode_ls_cons_arr("m", f, 0, 0), _umode_ls_cons_i("i", 0));
            asm volatile(_umode_ls_ops("flq", "f") : : _umode_ls_cons_arr("m", f, 0, 8), _umode_ls_cons_i("i", 8));
            asm volatile(_umode_ls_ops("flq", "f") : : _umode_ls_cons_arr("m", f, 0, 16), _umode_ls_cons_i("i", 16));
            asm volatile(_umode_ls_ops("flq", "f") : : _umode_ls_cons_arr("m", f, 0, 24), _umode_ls_cons_i("i", 24));
            csr_write(CSR_FCSR, fcsr);
            _umode_ext_disable();
        }
        else
        {
            // Do nothing
        }
    }
};

umode_float_ctx_t<64> umode_single_float_ctx;

__attribute__((constructor())) void __()
{
    umode_single_float_ctx.save();
    umode_single_float_ctx.restore();
}