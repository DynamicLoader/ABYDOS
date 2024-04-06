#ifndef __K_VMMGR_H__
#define __K_VMMGR_H__

#include "k_mmu.h"


class VMemoryMgr
{
  public:
    VMemoryMgr(MMUBase *mmu) : _mmu(mmu)
    {
        _maps = _global_maps;
    }

    void addMap(uintptr_t vaddr, uintptr_t paddr, size_t size, int prot)
    {
        if(prot & MMUBase::PROT_G)
            _global_maps.push_back({vaddr, paddr, size, prot, map_t::MAP});
        _maps.push_back({vaddr, paddr, size, prot, map_t::MAP});
    }

    // Actually do the map and unmap, note that we don't call apply() here
    int confirm()
    {
        int rc = 0;
        for (auto &map : _maps)
        {
            switch(map.pending){
                case map_t::MAP:
                    rc = _mmu->map(map.vaddr, map.paddr, map.size, map.prot);
                    // Todo: sync global map
                    break;
                case map_t::UNMAP:
                    rc = _mmu->unmap(map.vaddr, map.size);
                    // Todo delete
                    break;
                default:
                    continue;
            };
            if(rc < 0)
                return rc;
            map.pending = map_t::NONE;
        }
        return rc;
    }

    MMUBase* getMMU()
    {
        return _mmu;
    }

  private:
    struct map_t
    {
        uintptr_t vaddr;
        uintptr_t paddr;
        size_t size;
        int prot;
        enum pending_t {
            NONE = 0,
            MAP,
            UNMAP
        } pending = NONE;
    };
    std::vector<map_t> _maps;

    static std::vector<map_t> _global_maps;
    MMUBase *_mmu;
};


#endif