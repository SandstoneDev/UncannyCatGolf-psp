#pragma once

#include "visual.hpp"
#include <cstdint>
#include <cstdio>
#include <pspkernel.h>

class BillboardArt {
public:
    static constexpr unsigned Count=9, Size=128;

    ~BillboardArt() { if (file) std::fclose(file); }

    bool open(const char* path) {
        std::fill(std::begin(cached),std::end(cached),MaxFrames);
        file=std::fopen(path,"rb");
        uint32_t magic=0,total=0;
        if (!file || std::fread(&magic,sizeof magic,1,file)!=1 || magic!=0x42424355 ||
            std::fread(animations,sizeof animations,1,file)!=1) return false;
        for (const auto& animation:animations) {
            if (animation.first!=total || !animation.count || animation.count>MaxFrames-total) return false;
            total+=animation.count;
        }
        offset=4+sizeof animations+total*sizeof(Frame);
        if (std::fread(frames,sizeof(Frame),total,file)!=total || std::fseek(file,0,SEEK_END)!=0 ||
            std::ftell(file)!=static_cast<long>(offset+total*FrameBytes)) return false;
        for (const auto& animation:animations) {
            unsigned end=0;
            for (unsigned i=0;i<animation.count;++i) {
                const Frame& frame=frames[animation.first+i];
                const Patch& p=frame.patch;
                if (frame.end<=end || p.u!=0 || p.v!=0 || p.width<=0 || p.height<=0 ||
                    p.width>static_cast<int>(Size) || p.height>static_cast<int>(Size) ||
                    p.x<136 || p.x+p.width>408 || p.y<0 || p.y+p.height>272) return false;
                end=frame.end;
            }
        }
        return true;
    }

    const uint32_t* frame(unsigned screen,unsigned ticks,Patch& patch) {
        if (screen>=Count) return nullptr;
        const Animation& animation=animations[screen];
        const unsigned phase=ticks%frames[animation.first+animation.count-1].end;
        unsigned index=animation.first;
        while (frames[index].end<=phase) ++index;
        const unsigned slot=screen%Slots;
        if (cached[slot]!=index) {
            if (std::fseek(file,offset+index*FrameBytes,SEEK_SET)!=0 ||
                std::fread(pixels[slot],FrameBytes,1,file)!=1) {
                cached[slot]=MaxFrames;
                return nullptr;
            }
            sceKernelDcacheWritebackRange(pixels[slot],FrameBytes);
            cached[slot]=index;
        }
        patch=frames[index].patch;
        return pixels[slot];
    }

private:
    // No course draws more than two consecutive billboard tracks in one GE frame.
    static constexpr unsigned Slots=2, MaxFrames=512, FrameBytes=Size*Size*4;
    struct Animation { uint32_t first,count; };
    struct Frame { Patch patch; uint32_t end; };
    static_assert(sizeof(Frame)==16);
    Animation animations[Count]{};
    Frame frames[MaxFrames]{};
    std::FILE* file=nullptr;
    unsigned offset=0,cached[Slots]{};
    alignas(64) uint32_t pixels[Slots][Size*Size]{};
};
