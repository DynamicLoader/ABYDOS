#ifndef __K_UMODE_H__
#define __K_UMODE_H__

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

struct saved_context_t
{
    // important registers
    unsigned long ra;
    unsigned long gp;
    unsigned long tp;
    // temp registers
    unsigned long t0;
    unsigned long t1;
    unsigned long t2;
    unsigned long t3;
    unsigned long t4;
    unsigned long t5;
    unsigned long t6;
    // argument registers
    unsigned long a0;
    unsigned long a1;
    unsigned long a2;
    unsigned long a3;
    unsigned long a4;
    unsigned long a5;
    unsigned long a6;
    unsigned long a7;
};

struct umode_basic_ctx_t
{
    unsigned long ra, gp, tp;
    unsigned long t[7];
    unsigned long a[8];
    // Additional registers to saved_context_t
    unsigned long s[12];
    unsigned long sp, pc;

    operator saved_context_t(){
        return *(saved_context_t*)this;
    }
};

template <uint8_t flen> struct umode_float_ctx_t
{
    uint32_t f[32 * (flen >> 5)];
    uint32_t fcsr;

#define _f_save(ext, len, ls)                                                                                          \
    do                                                                                                                 \
    {                                                                                                                  \
        _umode_ext_enable(#ext);                                                                                       \
        asm volatile(_umode_ls_ops("fs" #len, "f") : _umode_ls_cons_arr("=m", f, ls, 0) : _umode_ls_cons_i("i", 0));   \
        asm volatile(_umode_ls_ops("fs" #len, "f") : _umode_ls_cons_arr("=m", f, ls, 8) : _umode_ls_cons_i("i", 8));   \
        asm volatile(_umode_ls_ops("fs" #len, "f") : _umode_ls_cons_arr("=m", f, ls, 16) : _umode_ls_cons_i("i", 16)); \
        asm volatile(_umode_ls_ops("fs" #len, "f") : _umode_ls_cons_arr("=m", f, ls, 24) : _umode_ls_cons_i("i", 24)); \
        fcsr = csr_read(CSR_FCSR);                                                                                     \
        _umode_ext_disable();                                                                                          \
    } while (0)

#define _f_load(ext, len, ls)                                                                                          \
    do                                                                                                                 \
    {                                                                                                                  \
        _umode_ext_enable(#ext);                                                                                       \
        asm volatile(_umode_ls_ops("fl" #len, "f") : : _umode_ls_cons_arr("m", f, ls, 0), _umode_ls_cons_i("i", 0));   \
        asm volatile(_umode_ls_ops("fl" #len, "f") : : _umode_ls_cons_arr("m", f, ls, 8), _umode_ls_cons_i("i", 8));   \
        asm volatile(_umode_ls_ops("fl" #len, "f") : : _umode_ls_cons_arr("m", f, ls, 16), _umode_ls_cons_i("i", 16)); \
        asm volatile(_umode_ls_ops("fl" #len, "f") : : _umode_ls_cons_arr("m", f, ls, 24), _umode_ls_cons_i("i", 24)); \
        csr_write(CSR_FCSR, fcsr);                                                                                     \
        _umode_ext_disable();                                                                                          \
    } while (0)

    // must be executed after main context is saved
    void save()
    {

        if constexpr (flen == 32)
        {
            _f_save(f, w, 0);
        }
        else if constexpr (flen == 64)
        {
            _f_save(d, d, 1);
        }
        else if constexpr (flen == 128)
        {
            _f_save(q, q, 2);
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
            _f_load(f, w, 0);
        }
        else if constexpr (flen == 64)
        {
            _f_load(d, d, 1);
        }
        else if constexpr (flen == 128)
        {
            _f_load(q, q, 2);
        }
        else
        {
            // Do nothing
        }
    }
};

template <uint16_t VLEN> struct umode_vector_ctx_t
{
    uint32_t v[32 * (VLEN >> 5)];
    unsigned long vstart, vcsr, vl, vtype; // unsigned long is always XLEN bits
};

// umode_float_ctx_t<64> umode_single_float_ctx;

// __attribute__((constructor())) void __()
// {
//     umode_single_float_ctx.save();
//     umode_single_float_ctx.restore();
// }

#endif // __K_UMODE_H__