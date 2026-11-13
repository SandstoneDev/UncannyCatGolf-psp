#pragma once

#include "game.hpp"
#include <pspkernel.h>

struct PoolRun {
    bool swimming=false, showingSpeech=false;
    unsigned speechStart=0;
    struct Ripple { Vec2 position{}; float scale=0; unsigned tick=0; };
    Ripple ripples[4]{};

    bool touch(Vec2 position,float radius,bool inWater,bool canSwim,unsigned tick,float speed) {
        swimming=inWater && canSwim;
        if (!swimming) return false;
        if (tick%6==0) {
            auto& ripple=ripples[(tick/6)%std::size(ripples)];
            ripple={position,0.5f*std::clamp((speed-80.0f)/(1000.0f-80.0f),0.0f,1.0f),tick};
        }
        const Vec2 offset=position-Vec2{368,-244};
        if (showingSpeech || std::abs(offset.x)>48+radius || std::abs(offset.y)>172+radius) return false;
        showingSpeech=true;
        speechStart=tick;
        return true;
    }
};

struct PoolAssets {
    static constexpr int Width=64, Height=128, Frames=16, SpeechWidth=256, SpeechHeight=256;
    uint8_t lineWidth[2][7]{};
    alignas(64) uint32_t sprites[256*64]{};
    alignas(64) uint32_t speech[SpeechWidth*SpeechHeight]{};
    alignas(64) uint16_t water[Frames][Width*Height]{};

    bool load(const char* path) {
        std::FILE* file=std::fopen(path,"rb");
        if (!file) return false;
        uint32_t magic=0;
        const bool ok=std::fread(&magic,sizeof magic,1,file)==1 && magic==0x31504355 &&
                      std::fread(lineWidth,sizeof lineWidth,1,file)==1 &&
                      std::fread(sprites,sizeof sprites,1,file)==1 &&
                      std::fread(speech,sizeof speech,1,file)==1 &&
                      std::fread(water,sizeof water,1,file)==1 &&
                      std::fgetc(file)==EOF && !std::ferror(file);
        std::fclose(file);
        if (!ok) return false;
        for (const auto& bank:lineWidth) {
            if (!bank[0]) return false;
            for (unsigned width:bank) if (width>188) return false;
        }
        sceKernelDcacheWritebackRange(sprites,sizeof sprites);
        sceKernelDcacheWritebackRange(speech,sizeof speech);
        sceKernelDcacheWritebackRange(water,sizeof water);
        return true;
    }
};
