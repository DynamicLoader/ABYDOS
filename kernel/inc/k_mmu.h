#ifndef __K_MMU_H__
#define __K_MMU_H__

#include "k_sysdev.h"
#include "k_mem.h"
#include "riscv_asm.h"
#include "sbi/riscv_encoding.h"

class MMUBase
{
  public:
    static constexpr int PROT_NONE = 0, PROT_R = 1, PROT_W = 2, PROT_X = 4, PROT_U = 8, PROT_G = 16;

    /**
     * @brief Set MMU state
     * @note The function will take effort immediately!
     * @param enable true to enable, false to disable
     * @return true if success, false if failed

     */
    virtual bool enable(bool enable) = 0;

    /**
     * @brief switch the ASID to this
     * @note The function will not sfence!
     */
    virtual void switchASID() = 0;

    virtual void getSATP(void *) = 0;

    virtual int map(uintptr_t vaddr, uintptr_t paddr, size_t size, int prot) = 0;
    virtual int unmap(uintptr_t vaddr, size_t size) = 0;

    virtual void apply() = 0;

    virtual size_t getVMALowerTop() = 0;
    virtual size_t getVMAUpperBottom() = 0;
};

class RV64MMUBase : public MMUBase
{

  public:
    bool enable(bool enable) override
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

    void switchASID() override
    {
        csr_write(CSR_SATP, *(uint64_t *)(&_satp));
        // No need to sfence.vma
    }

  protected:
    struct vaddr_t
    {
        uint64_t offset : 12;
        uint64_t vpn0 : 9;
        uint64_t vpn1 : 9;
        uint64_t vpn2 : 9;
        uint64_t vpn3 : 9;
        uint64_t vpn4 : 9;

        template <uint8_t sz> uint64_t getVPN(int level)
        {
            switch (level)
            {
            case 0:
                if constexpr (sz == 39)
                    return vpn2;
                else if constexpr (sz == 48)
                    return vpn3;
                else if constexpr (sz == 57)
                    return vpn4;
                else
                    return 0;
                break;
            case 1:
                if constexpr (sz == 39)
                    return vpn1;
                else if constexpr (sz == 48)
                    return vpn2;
                else if constexpr (sz == 57)
                    return vpn3;
                else
                    return 0;
                break;
            case 2:
                if constexpr (sz == 39)
                    return vpn0;
                else if constexpr (sz == 48)
                    return vpn1;
                else if constexpr (sz == 57)
                    return vpn2;
                else
                    return 0;
                break;
            case 3:
                if constexpr (sz == 39)
                    return 0;
                else if constexpr (sz == 48)
                    return vpn0;
                else if constexpr (sz == 57)
                    return vpn1;
                else
                    return 0;
                break;
            case 4:
                if constexpr (sz == 39)
                    return 0;
                else if constexpr (sz == 48)
                    return 0;
                else if constexpr (sz == 57)
                    return vpn0;
                else
                    return 0;
                break;
            default:
                return 0;
                break;
            }
        }
    };

    struct paddr_t
    {
        uint64_t offset : 12;
        uint64_t ppn0 : 9;
        uint64_t ppn1 : 9;
        uint64_t ppn2 : 9;
        uint64_t ppn3 : 9;
        uint64_t ppn4 : 8;
        uint64_t reserved : 8;
    };

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

        void ppn(uintptr_t addr)
        {
            auto paddr = (paddr_t *)&addr;
            ppn0 = paddr->ppn0;
            ppn1 = paddr->ppn1;
            ppn2 = paddr->ppn2;
            ppn3 = paddr->ppn3;
            ppn4 = paddr->ppn4;
        }

        uintptr_t paddr()
        {
            return (*((uintptr_t *)this) << 2) & ~((0xFFFUL) + (0xFFUL << 56));
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

    void getSATP(void * ret) override
    {
        *(satp_t *)ret = _satp;
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
    }
    ~RV64MMU()
    {
        printf("Freeing ptes at %lx\n", (uintptr_t)_ptes);
        alignedFree(_ptes);
    }

    size_t getVMALowerTop() override
    {
        if constexpr (sz == 39)
            return (1ULL << 38);
        else if constexpr (sz == 48)
            return (1ULL << 47);
        else if constexpr (sz == 57)
            return (1ULL << 56);
        else
            return 0;
    }

    size_t getVMAUpperBottom() override
    {
        if constexpr (sz == 39)
            return -1ULL - (1ULL << 38) + 1;
        else if constexpr (sz == 48)
            return -1ULL - (1ULL << 47) + 1;
        else if constexpr (sz == 57)
            return -1ULL - (1ULL << 56) + 1;
        else
            return 0;
    }

    int map(uintptr_t vaddr, uintptr_t paddr, size_t size, int prot) override
    {
        if (vaddr & 0xFFF || paddr & 0xFFF || size & 0xFFF) // Not aligned
            return K_EINVAL;
        if (size == 0)
            return 0;                                     // No need to map with size == 0
        if (vaddr + size < vaddr || paddr + size < paddr) // overflow
            return K_EINVALID_ADDR;
        // if (vaddr + size > (1ULL << sz) || paddr + size > (1ULL << sz))
        //     return -1;

        auto prott = prot & (PROT_R | PROT_W | PROT_X);
        if (prott == 0b000 || prott == 0b010 || prott == 0b110)
            return K_ENOSPC;

        auto rc = 0;
        // Divide the memory into blocks of size 256T，512G, 1G, 2M, and 4K
        for (uintptr_t vcaddr = vaddr, pcaddr = paddr; vcaddr < vaddr + size;)
        {
            if (rc)
                return rc;
            if constexpr (sz >= 57) // Only SV57 and SV64 support 256T
            {
                if ((vcaddr & 0xFFFFFFFFFFFF) == 0 && (pcaddr & 0xFFFFFFFFFFFF) == 0) // 256T aligned
                {
                    if (size - (vcaddr - vaddr) >= 1ULL << 48) // There are more than 256T to map
                    {
                        rc = _map<48>(vcaddr, pcaddr, prot);
                        vcaddr += 1ULL << 48;
                        pcaddr += 1ULL << 48;
                        continue;
                    }
                }
            }

            if constexpr (sz >= 48) // Only SV48, SV57 and SV64 support 512G
            {
                if ((vcaddr & 0x7FFFFFFFFF) == 0 && (pcaddr & 0x7FFFFFFFFF) == 0) // 512G aligned
                {
                    if (size - (vcaddr - vaddr) >= 1ULL << 39) // There are more than 512G to map
                    {
                        rc = _map<39>(vcaddr, pcaddr, prot);
                        vcaddr += 1ULL << 39;
                        pcaddr += 1ULL << 39;
                        continue;
                    }
                }
            }

            if ((vcaddr & 0x3FFFFFFF) == 0 && (pcaddr & 0x3FFFFFFF) == 0) // 1G aligned
            {
                if (size - (vcaddr - vaddr) >= 1ULL << 30) // There are more than 1G to map
                {
                    rc = _map<30>(vcaddr, pcaddr, prot);
                    vcaddr += 1ULL << 30;
                    pcaddr += 1ULL << 30;
                    continue;
                }
            }

            if ((vcaddr & 0x1FFFFF) == 0 && (pcaddr & 0x1FFFFF) == 0) // 2M aligned
            {
                if (size - (vcaddr - vaddr) >= 1ULL << 21) // There are more than 2M to map
                {
                    rc = _map<21>(vcaddr, pcaddr, prot);
                    vcaddr += 1ULL << 21;
                    pcaddr += 1ULL << 21;
                    continue;
                }
            }

            if ((vcaddr & 0xFFF) == 0 && (pcaddr & 0xFFF) == 0) // 4K aligned
            {
                if (size - (vcaddr - vaddr) >= 1ULL << 12) // There are more than 4K to map
                {
                    rc = _map<12>(vcaddr, pcaddr, prot);
                    vcaddr += 1ULL << 12;
                    pcaddr += 1ULL << 12;
                    continue;
                }
            }

            return K_EINVALID_ADDR;
        }
        printf("Mapped %lx to %lx with prot %i\n", vaddr, paddr, prot);
        return rc;
    }

    int unmap(uintptr_t vaddr, size_t size) override
    {
        if (vaddr & 0xFFF || size & 0xFFF) // Not aligned
            return K_EINVAL;
        if (size == 0)
            return 0;
        if (vaddr + size < vaddr) // overflow
            return K_EINVALID_ADDR;

        auto rc = 0;

        for (uintptr_t vcaddr = vaddr; vcaddr < vaddr + size;)
        {
            if (rc)
                return rc;
            if constexpr (sz >= 57) // Only SV57 and SV64 support 256T
            {
                if ((vcaddr & 0xFFFFFFFFFFFF) == 0) // 256T aligned
                {
                    if (size - (vcaddr - vaddr) >= 1ULL << 48) // There are more than 256T to map
                    {
                        rc = _unmap<48>(vcaddr);
                        vcaddr += 1ULL << 48;
                        continue;
                    }
                }
            }

            if constexpr (sz >= 48) // Only SV48, SV57 and SV64 support 512G
            {
                if ((vcaddr & 0x7FFFFFFFFF) == 0) // 512G aligned
                {
                    if (size - (vcaddr - vaddr) >= 1ULL << 39) // There are more than 512G to map
                    {
                        rc = _unmap<39>(vcaddr);
                        vcaddr += 1ULL << 39;
                        continue;
                    }
                }
            }

            if ((vcaddr & 0x3FFFFFFF) == 0) // 1G aligned
            {
                if (size - (vcaddr - vaddr) >= 1ULL << 30) // There are more than 1G to map
                {
                    rc = _unmap<30>(vcaddr);
                    vcaddr += 1ULL << 30;
                    continue;
                }
            }

            if ((vcaddr & 0x1FFFFF) == 0) // 2M aligned
            {
                if (size - (vcaddr - vaddr) >= 1ULL << 21) // There are more than 2M to map
                {
                    rc = _unmap<21>(vcaddr);
                    vcaddr += 1ULL << 21;
                    continue;
                }
            }

            if ((vcaddr & 0xFFF) == 0) // 4K aligned
            {
                if (size - (vcaddr - vaddr) >= 1ULL << 12) // There are more than 4K to map
                {
                    rc = _unmap<12>(vcaddr);
                    vcaddr += 1ULL << 12;
                    continue;
                }
            }

            return K_EINVALID_ADDR;
        }
        printf("Unmapped %lx\n", vaddr);
        return rc;
    }

    void apply() override
    {
        sfence_vma();
    }

  protected:
    static constexpr int _getMaxLevel()
    {
        if constexpr (sz == 39)
            return 2;
        else if constexpr (sz == 48)
            return 3;
        else if constexpr (sz == 57)
            return 4;
        else
            return K_EINVAL;
    }

    template <uint8_t blocksz> int _calcLevel()
    {
        auto level = 0;
        if constexpr (blocksz == 12) // 4K
            level = 0;
        else if constexpr (blocksz == 21) // 2M
            level = -1;
        else if constexpr (blocksz == 30) // 1G
            level = -2;
        else if constexpr (blocksz == 39) // 512G
            level = -3;
        else if constexpr (blocksz == 48) // 256T
            level = -4;
        else
            return K_EINVAL;

        level += _getMaxLevel();

        if (level < 0)
            return K_EINVAL;

        return level;
    }

    pte_t *_createPTE(int level, uintptr_t vaddr)
    {
        auto poff = ((vaddr_t *)&vaddr)->getVPN<sz>(level);
        // printf("Creating PTE for %lx @ L%i with poff = %li\n", vaddr, level, poff);
        if (level == 0)
            return _ptes + poff;                    // We have already created the root level
        auto parent = _createPTE(level - 1, vaddr); // Create parent PTE first
        // printf("Original Parent PTE has value %lx\n", *(uint64_t *)parent);

        pte_t *thisPTE = nullptr; // This level PTE 's base address
        // printf("Parent PTE.paddr = 0x%lx\n", parent->paddr());
        if (parent->paddr() != 0) // this level already created, get base
        {
            thisPTE = (pte_t *)(parent->paddr());
        }
        else
        {
            thisPTE = alignedMalloc<pte_t>(512 * sizeof(pte_t), 4096);
            parent->ppn((uintptr_t)thisPTE);
            parent->v = 1;
            parent->r = 0;
            parent->w = 0;
            parent->x = 0; // mark as a pointer
            parent->template fit<sz>();
            // printf("Created new PTE at %lx\n", (uintptr_t)thisPTE);
            // printf("Now Parent PTE has value %lx\n", *(uint64_t *)parent);
        }
        return thisPTE + poff;
    }

    pte_t *_getPTE(int level, uintptr_t vaddr)
    {
        auto poff = ((vaddr_t *)&vaddr)->getVPN<sz>(level);
        if (level == 0)
            return _ptes + poff;
        auto parent = _getPTE(level - 1, vaddr);
        return (pte_t *)(parent->paddr()) + poff;
    }

    int _removePTE(int level, uintptr_t vaddr)
    {
        auto pte = _getPTE(level, vaddr);
        if (!pte->v)
            return K_EALREADY;
        pte->v = 0;
        pte->ppn(0);
        pte->template fit<sz>();
        if (level != _getMaxLevel())
        { // Next level already unused, free it

            auto pteBase = (pte_t *)(pte->paddr());
            printf("removing pte memory at %lx\n", (uintptr_t)pteBase);
            alignedFree(pteBase);
        }
        return (level == 0 ? 0 : _removePTE(level - 1, vaddr));
    }

    template <uint8_t blocksz> int _map(uintptr_t vaddr, uintptr_t paddr, int prot)
    {
        // printf("* Mapping %lx to %lx with prot %i\n", vaddr, paddr, prot);

        auto level = _calcLevel<blocksz>();
        if (level < 0)
            return level;

        auto pte = _createPTE(level, vaddr);
        // printf("PTE got: %lx\n", (uintptr_t)pte);

        if (pte->v)
            return K_EALREADY;
        pte->v = 1;
        pte->r = prot & PROT_R ? 1 : 0;
        pte->w = prot & PROT_W ? 1 : 0;
        pte->x = prot & PROT_X ? 1 : 0;
        pte->u = prot & PROT_U ? 1 : 0;
        pte->g = prot & PROT_G ? 1 : 0;

        pte->ppn(paddr);
        pte->template fit<sz>();
        // printf("Now PTE value: %lx\n", *(uintptr_t *)pte);
        return 0;
    }

    template <uint8_t blocksz> int _unmap(uintptr_t vaddr)
    {
        // printf("* Unmapping %lx\n", vaddr);
        auto level = _calcLevel<blocksz>();

        if (level < 0)
            return level;

        // return _removePTE(level, vaddr);
        auto pte = _getPTE(level, vaddr);
        if (!pte->v)
            return K_EALREADY;
        pte->v = 0;
        pte->ppn(0);
        pte->template fit<sz>();
        return 0;
    }

  private:
    pte_t *_ptes; // Root level page table
};

using SV39MMU = RV64MMU<39>;
using SV48MMU = RV64MMU<48>;
using SV57MMU = RV64MMU<57>;

#endif
