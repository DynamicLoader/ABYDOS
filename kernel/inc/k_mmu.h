#ifndef __K_MMU_H__
#define __K_MMU_H__

#include "k_sysdev.h"
#include "k_mem.h"
#include "riscv_asm.h"
#include "sbi/riscv_encoding.h"

class MMUBase
{
  public:
    virtual bool enable(bool enable) = 0;
};

class RV64MMUBase : public MMUBase
{

  public:
    bool enable(bool enable)
    {
        if (enable)
        {
            csr_write(CSR_SATP, *(uint64_t *)(&_satp));
            if (csr_read(CSR_SATP) != *(uint64_t *)(&_satp))
                return false;
            sfence_vma();
            return true;
        }
        else
        {
            csr_write(CSR_SATP, 0);
            sfence_vma();
            return true;
        }
    }

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
        else if constexpr (sz == 64)
            return MMUMode_t::SV64;
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
            _ptes[i].ppn2 = i;
            _ptes[i].template fit<sz>();
        }

        _ptes[255] = _ptes[2]; // stack
    }

  private:
    pte_t *_ptes; // Root level page table
};

using SV39MMU = RV64MMU<39>;
using SV48MMU = RV64MMU<48>;
using SV57MMU = RV64MMU<57>;

#endif
