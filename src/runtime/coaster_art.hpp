#pragma once

#include "asset_file.hpp"
#include "game.hpp"
#include "visual.hpp"

#include <cstring>
#include <new>
#include <utility>

#ifdef __PSP__
#include <pspkernel.h>
#endif

struct CoasterRun {
    static constexpr unsigned SoundTick=156;

    static constexpr unsigned startSoundTick(bool stage308) { return stage308 ? 164 : SoundTick; }
    static constexpr Vec2 courseShift(bool stage308) { return {0,stage308 ? 16.0f : 0.0f}; }
    static constexpr Vec2 TrainOrigin{-616,152};
    static constexpr Vec2 RiderCenters[]={
        {-328,168},{-304,168},{-304,184},{-328,184},
        {-416,168},{-392,168},{-392,184},{-416,184},
        {-496,168},{-472,168},{-496,184},{-472,184},
        {-560,168},{-560,184},{-584,168},{-584,184},
    };

    unsigned riders[std::size(RiderCenters)]{};

    void restart(uint32_t entropy) {
        for (auto& rider:riders) {
            entropy=entropy*1664525u+1013904223u;
            rider=(entropy>>16)%5;
        }
    }

    static constexpr float offset(unsigned tick) {
        const unsigned travelTick=std::clamp(tick,180u,300u)-180;
        return travelTick*(880.0f/120.0f);
    }
};

class CoasterArt {
public:
    static constexpr unsigned Width=512, Height=128;
    static constexpr Patch Train{0,0,344,72,0,0};

    static constexpr Patch rider(unsigned frame) {
        return {static_cast<int16_t>(344+32*frame),0,32,32,-16,-16};
    }

    bool load(const char* path,void* video=nullptr,void* scratch=nullptr) {
        unload();
        const auto input=openAssetFile(path);
        auto* file=input.get();
        char magic[4]{};
        if (!file || std::fread(magic,sizeof magic,1,file)!=1 ||
            std::memcmp(magic,"UCR1",sizeof magic)!=0) return false;

        std::unique_ptr<Atlas> loaded;
        if (!video) loaded.reset(new (std::nothrow) Atlas);
        auto* pixelData=video?scratch:loaded?loaded->pixels:nullptr;
        constexpr unsigned bytes=Width*Height*sizeof(uint32_t);
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
        borrowed=static_cast<const uint32_t*>(video);
        atlas=std::move(loaded);
        return true;
    }

    void unload() { atlas.reset(); borrowed=nullptr; }
    const uint32_t* pixels() const { return borrowed?borrowed:atlas?atlas->pixels:nullptr; }

private:
    struct alignas(64) Atlas { uint32_t pixels[Width*Height]; };
    std::unique_ptr<Atlas> atlas;
    const uint32_t* borrowed=nullptr;
};

