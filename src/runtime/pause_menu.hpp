#pragma once

#include <cstdio>
#include <cstring>
#include <pspctrl.h>
#include <pspkernel.h>

#include "pause_art.hpp"

struct PauseMenuState {
    enum class Choice : unsigned { Resume, Settings, LevelSelect, Main, None };

    Choice choice=Choice::Resume;
    bool crossArmed=false;
    unsigned elapsedMs=0;

    Choice update(uint32_t pressed,uint32_t released,bool clean) {
        if (clean) {
            crossArmed=false;
            return Choice::None;
        }
        if (pressed&PSP_CTRL_CROSS) crossArmed=true;
        const bool up=pressed&PSP_CTRL_UP,down=pressed&PSP_CTRL_DOWN;
        if (!crossArmed && up!=down) {
            const unsigned row=static_cast<unsigned>(choice);
            choice=static_cast<Choice>((row+(down?1:3))%4);
        }
        if (!crossArmed || !(released&PSP_CTRL_CROSS)) return Choice::None;
        crossArmed=false;
        return choice;
    }
};

namespace PauseArt {
constexpr int Field=272;
constexpr unsigned FilterFrames=10, MaxDots=1024;
alignas(64) uint32_t atlas[Width*Height]{};
alignas(64) uint32_t colors[256]{};
alignas(64) uint8_t darkness[Field*Field]{}, filter[Field*Field]{};
uint32_t counts[FilterFrames]{}, dots[FilterFrames][MaxDots]{};
unsigned cached=FilterFrames;

bool load(const char* path) {
    auto* file=std::fopen(path,"rb");
    if (!file) return false;
    uint32_t magic=0;
    bool ok=std::fread(&magic,sizeof magic,1,file)==1 && magic==0x50504355 &&
                  std::fread(atlas,sizeof atlas,1,file)==1 &&
                  std::fread(colors,sizeof colors,1,file)==1 &&
                  std::fread(darkness,sizeof darkness,1,file)==1 &&
                  std::fread(counts,sizeof counts,1,file)==1;
    for (unsigned frame=0;ok && frame<FilterFrames;++frame) {
        const unsigned count=counts[frame];
        ok=count<=MaxDots && std::fread(dots[frame],sizeof(uint32_t),count,file)==count;
        unsigned nextOffset=0;
        for (unsigned i=0;ok && i<count;++i) {
            const uint32_t dot=dots[frame][i];
            const unsigned offset=dot>>8;
            ok=offset>=nextOffset && offset<sizeof filter && (dot&0xff);
            nextOffset=offset+1;
        }
    }
    ok=ok && std::fgetc(file)==EOF && !std::ferror(file);
    std::fclose(file);
    if (ok) {
        sceKernelDcacheWritebackRange(atlas,sizeof atlas);
        sceKernelDcacheWritebackRange(colors,sizeof colors);
        sceKernelDcacheWritebackRange(darkness,sizeof darkness);
        cached=FilterFrames;
    }
    return ok;
}

const uint8_t* filterPixels(unsigned frame) {
    if (cached!=frame) {
        std::memset(filter,0,sizeof filter);
        for (unsigned i=0;i<counts[frame];++i) {
            const uint32_t dot=dots[frame][i];
            filter[dot>>8]=static_cast<uint8_t>(dot);
        }
        sceKernelDcacheWritebackRange(filter,sizeof filter);
        cached=frame;
    }
    return filter;
}
} // namespace PauseArt
