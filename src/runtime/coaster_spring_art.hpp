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

    bool load(const char* path,void* video=nullptr,void* scratch=nullptr) {
        const auto input=openAssetFile(path);
        auto* file=input.get();
        char magic[4]{};
        if (!file || std::fread(magic,sizeof magic,1,file)!=1 ||
            std::memcmp(magic,"UCS8",sizeof magic)!=0) return false;
        Patch next[Springs*Frames]{};
        if (std::fread(next,sizeof next,1,file)!=1) return false;
        for (const auto& p:next) {
            if (p.u<0 || p.v<0 || p.width<=0 || p.height<=0 || p.x<136 || p.y<0 ||
                p.u+p.width>static_cast<int>(Width) || p.v+p.height>static_cast<int>(Height) ||
                p.x+p.width>408 || p.y+p.height>272) return false;
        }
        std::unique_ptr<Atlas> memory;
        if (!video) memory.reset(new (std::nothrow) Atlas);
        auto* pixelData=video?scratch:memory?memory->pixels:nullptr;
        constexpr unsigned bytes=Width*Height*sizeof(uint16_t);
        if (!pixelData || std::fread(pixelData,bytes,1,file)!=1 ||
            std::fgetc(file)!=EOF || std::ferror(file)) return false;
#ifdef __PSP__
        if (video) {
            auto* uncached=reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(video)|0x40000000u);
            std::memcpy(uncached,pixelData,bytes);
        } else sceKernelDcacheWritebackRange(pixelData,bytes);
#else
        if (video) std::memcpy(video,pixelData,bytes);
#endif
        std::copy(std::begin(next),std::end(next),patches);
        borrowed=static_cast<const uint16_t*>(video);
        atlas=std::move(memory);
        return true;
    }

    void unload() { atlas.reset(); borrowed=nullptr; }
    const uint16_t* pixels() const { return borrowed?borrowed:atlas?atlas->pixels:nullptr; }
    const Patch& patch(unsigned spring,unsigned tick) const { return patches[spring*Frames+frame(spring,tick)]; }

private:
    struct Atlas {
        alignas(64) uint16_t pixels[Width*Height];
    };
    std::unique_ptr<Atlas> atlas;
    Patch patches[Springs*Frames]{};
    const uint16_t* borrowed=nullptr;
};
