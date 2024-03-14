#ifndef __K_MMU_H__
#define __K_MMU_H__

#include "k_sysdev.h"
#include "k_mem.h"
#include "riscv_asm.h"
#include "sbi/riscv_encoding.h"

class MMUBase
{
  public:
};

class RV64MMUBase : public MMUBase
{

  protected:
    enum MMUMode_t
    {
        BARE = 0,
        // 1-7 reserved for future use
        SV39 = 8,
        SV48 = 9,
        SV57 = 10,
        SV64 = 11, // Not defined in the current RISC-V specs
    };

    struct pte_t
    {
        uint64_t v : 1;
        uint64_t r : 1;
        uint64_t w : 1;
        uint64_t x : 1;
        uint64_t u : 1;
        uint64_t g : 1;
        uint64_t a : 1;
        uint64_t d : 1;
        uint64_t rsw : 2;
        uint64_t ppn0 : 9;
        uint64_t ppn1 : 9;
        uint64_t ppn2 : 9;
        uint64_t ppn3 : 9;
        uint64_t ppn4 : 8;
        uint64_t reserved : 10; // externsions off

        // C++ 17 enabled!
        template <uint8_t sz> auto fit()
        {
            this->reserved = 0;
            if constexpr (sz <= 39)
                this->ppn3 = 0;
            if constexpr (sz <= 48)
                this->ppn4 = 0;
            return *this;
        }
    };

    struct satp_t
    {
        uint64_t ppn : 44;
        uint64_t asid : 16;
        uint64_t mode : 4;
    };

    RV64MMUBase(MMUMode_t mode, uint16_t asid)
    {
        _satp.asid = asid;
        _satp.mode = mode;
    }

    bool setPPN(uintptr_t addr)
    {
        if (addr & 4095)
            return false;
        _satp.ppn = addr2page4K(addr);
        return true;
    }

    void setState(bool enable)
    {
        if (enable)
        {
            csr_write(CSR_SATP, *(uint64_t *)(&_satp));
            sfence_vma();
        }
        else
        {
            csr_write(CSR_SATP, 0);
            sfence_vma();
        }
    }

    /**
     * @brief switch the ASID to this
     */
    void switchASID()
    {
        csr_write(CSR_SATP, *(uint64_t *)(&_satp));
        // No need to sfence.vma
    }

  private: // data
    satp_t _satp;
};

template <uint8_t sz> class RV64MMU : public RV64MMUBase
{
  private:
    static constexpr auto _mmutype()
    {
        if constexpr (sz == 39)
            return MMUMode_t::SV39;
        else if constexpr (sz == 48)
            return MMUMode_t::SV48;
        else if constexpr (sz == 57)
            return MMUMode_t::SV57;
        else
            return MMUMode_t::BARE;
    }

  public:
    RV64MMU(uint16_t asid) : RV64MMUBase(_mmutype(), asid)
    {
        _ptes = alignedMalloc<pte_t>(512 * sizeof(pte_t), 4096);
        setPPN((uintptr_t)_ptes);
        // We allow lower 3G to be accessible for testing
        for (int i = 0; i < 4; ++i)
        {
            _ptes[i].v = 1;
            _ptes[i].r = 1;
            _ptes[i].w = 1;
            _ptes[i].x = 1;
            _ptes[i].ppn0 = 0;
            _ptes[i].ppn1 = 0; // 30bits here
            // We map the 4G from 3G space...
            _ptes[i].ppn2 = (i == 3 ? 2 : i);
            _ptes[i].template fit<sz>();
        }

        printf("Going to enable MMU...");
        fflush(stdout);
        setState(true);
        printf("OK!\n");

        int a = 114514;
        printf("Original addr of a: 0x%lx with value %i\n", (uintptr_t)&a, a);
        int *pa = (&a) + 0x40000000 / sizeof(int);
        printf("Mapped addr of a: 0x%lx with value %i\n", (uintptr_t)pa, *pa);

        *pa = 1919810;
        printf("We modified from mapped, now the original value is %i\n", a);
    }

  private:
    pte_t *_ptes; // Root level page table
};

using SV39MMU = RV64MMU<39>;
using SV48MMU = RV64MMU<48>;
using SV57MMU = RV64MMU<57>;

#endif
