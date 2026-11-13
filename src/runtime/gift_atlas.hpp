#pragma once

#include "asset_file.hpp"
#include "visual.hpp"
#include "selector_gifts.hpp"

#include <algorithm>
#include <cstdio>
#include <pspkernel.h>

namespace GiftArt {
constexpr unsigned tileRows(unsigned pixels) { return (pixels+7)&~7u; }
constexpr unsigned bottom(const Patch& patch) { return patch.v+patch.height; }

constexpr unsigned iconRows() {
    unsigned end=0;
    for (const auto& icon:SelectorGifts::Spin) end=std::max(end,bottom(icon));
    for (const auto& locale:SelectorGifts::Panels)
        for (const auto& panel:locale)
            for (const auto& part:panel) end=std::max(end,bottom(part));
    return tileRows(end);
}

constexpr unsigned explosionRows() {
    unsigned end=0;
    for (const auto& frame:SelectorGifts::Blast) end=std::max(end,bottom(frame.art));
    return tileRows(end);
}

constexpr unsigned IconRows=iconRows(), ExplosionRows=explosionRows();
static_assert(IconRows<=SelectorGifts::Height && ExplosionRows<=SelectorGifts::Height);
static_assert(SelectorGifts::Width*sizeof(uint32_t)%16==0);

// Each swizzled tile occupies eight rows. A prefix of complete tile rows has
// the same texel addresses and stride as the original 512-row atlas page.
inline bool load(const char* path,uint32_t* output,unsigned rows,unsigned pages=1) {
    constexpr long stride=SelectorGifts::Width*sizeof(uint32_t);
    constexpr long sourceBytes=stride*SelectorGifts::Height;
    if (!rows || rows>SelectorGifts::Height || rows%8 || !pages) return false;
    const auto input=openAssetFile(path);
    auto* file=input.get();
    if (!file || std::fseek(file,0,SEEK_END)!=0 ||
        std::ftell(file)!=sourceBytes*static_cast<long>(pages)) return false;
    const unsigned bytes=stride*rows;
    for (unsigned page=0;page<pages;++page) {
        auto* target=output+page*SelectorGifts::Width*rows;
        if (std::fseek(file,static_cast<long>(page)*sourceBytes,SEEK_SET)!=0 ||
            std::fread(target,bytes,1,file)!=1) return false;
        sceKernelDcacheWritebackRange(target,bytes);
    }
    return true;
}
} // namespace GiftArt
