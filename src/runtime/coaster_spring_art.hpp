#pragma once

#include "asset_file.hpp"
#include "visual.hpp"

#include <cstring>
#include <new>
#include <utility>

#ifdef __PSP__
#include <pspkernel.h>
#endif

class CoasterSpringArt {
public:
    static constexpr unsigned Width=256, Height=256, Springs=2, Frames=4;

    static constexpr unsigned frame(unsigned spring,unsigned tick) {
        // The blue sprite starts 0.529 of a frame ahead. The red starts at 0.083.
        const unsigned phase=spring==0 ? 2 : 0;
        return ((tick%16+phase)/4)%Frames;
    }

    bool load(const char* path) {
        const auto input=openAssetFile(path);
        auto* file=input.get();
        char magic[4]{};
        if (!file || std::fread(magic,sizeof magic,1,file)!=1 ||
            std::memcmp(magic,"UCS8",sizeof magic)!=0) return false;
        std::unique_ptr<Atlas> next(new (std::nothrow) Atlas);
        if (!next || std::fread(next->patches,sizeof next->patches,1,file)!=1) return false;
        for (const auto& p:next->patches) {
            if (p.u<0 || p.v<0 || p.width<=0 || p.height<=0 || p.x<136 || p.y<0 ||
                p.u+p.width>static_cast<int>(Width) || p.v+p.height>static_cast<int>(Height) ||
                p.x+p.width>408 || p.y+p.height>272) return false;
        }
        if (std::fread(next->pixels,sizeof next->pixels,1,file)!=1 ||
            std::fgetc(file)!=EOF || std::ferror(file)) return false;
#ifdef __PSP__
        sceKernelDcacheWritebackRange(next->pixels,sizeof next->pixels);
#endif
        atlas=std::move(next);
        return true;
    }

    void unload() { atlas.reset(); }
    const uint16_t* pixels() const { return atlas ? atlas->pixels : nullptr; }
    const Patch& patch(unsigned spring,unsigned tick) const { return atlas->patches[spring*Frames+frame(spring,tick)]; }

private:
    struct Atlas {
        Patch patches[Springs*Frames];
        alignas(64) uint16_t pixels[Width*Height];
    };
    std::unique_ptr<Atlas> atlas;
};
