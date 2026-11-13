#pragma once

#include "asset_file.hpp"
#include "visual.hpp"

#include <cstdint>
#include <cstring>

namespace MenuScroll {
constexpr unsigned HudWidth=136, ViewWidth=408, Height=272;
constexpr unsigned LargeWidth=256, StripWidth=16, TileRows=208;
constexpr unsigned TileBytes=LargeWidth*TileRows*2;
constexpr unsigned HudBytes=LargeWidth*Height*2;
constexpr unsigned ForegroundBytes=LargeWidth*Height*4;
constexpr unsigned StripBytes=StripWidth*Height*4;
constexpr unsigned RecordBytes=HudBytes+ForegroundBytes+StripBytes;
constexpr unsigned RecordCount=4;
constexpr unsigned ButtonWidth=512, ButtonRows=256, ButtonCount=4;
constexpr unsigned ButtonBytes=ButtonWidth*ButtonRows*4;
constexpr unsigned ButtonRecordBytes=ButtonCount*2*sizeof(Patch)+ButtonBytes;
constexpr unsigned FileBytes=4+TileBytes+RecordCount*RecordBytes+2*ButtonRecordBytes;
constexpr unsigned PeriodMs=40000;
constexpr unsigned BackX=137, BackY=1, BackWidth=42, BackHeight=23;
constexpr unsigned BackStride=64, BackRows=24, BackBytes=BackStride*BackRows*4;
constexpr unsigned BackFrames=59, BackFps=30;
constexpr unsigned BackOffset=192*ButtonWidth*4;
constexpr unsigned BackTableBytes=(BackFrames+1)*sizeof(uint32_t);
constexpr float Scale=17.0f/32.0f, TileSize=200*Scale;

static_assert(sizeof(Patch)==12 && FileBytes==2896068);

inline unsigned recordOffset(unsigned index) { return 4+TileBytes+index*RecordBytes; }
inline unsigned buttonOffset(unsigned locale) { return recordOffset(RecordCount)+locale*ButtonRecordBytes; }

struct Phase { float x,y; };

inline Phase phase(unsigned ms) {
    return {static_cast<float>(ms%(PeriodMs/2))*Scale/100,
            static_cast<float>(ms%PeriodMs)*Scale/200};
}

inline bool valid(const char* path) {
    const auto file=openAssetFile(path);
    uint32_t magic=0;
    return file && std::fread(&magic,sizeof magic,1,file.get())==1 && magic==0x374d4355 &&
           std::fseek(file.get(),0,SEEK_END)==0 && std::ftell(file.get())==static_cast<long>(FileBytes);
}

struct Textures {
    const void* tile;
    const void* hud;
    const void* foreground;
    const void* strip;
    void* back;
};

constexpr unsigned TitleSkyBytes=512*Height*2, TitleLayerBytes=512*104*2;
constexpr unsigned TitleFloorBytes=256*64, TitleFloorFrames=39, TitlePaletteBytes=1024;
constexpr unsigned TitleFloorOffset=TitleSkyBytes+3*TitleLayerBytes+TitlePaletteBytes;
constexpr unsigned TitleBytes=TitleFloorOffset+TitleFloorFrames*TitleFloorBytes;

inline float titleHillPhase(unsigned elapsedMs,unsigned speed) {
    return (static_cast<uint64_t>(elapsedMs)*speed%640000)*(Scale/1000);
}

inline bool decodeBackFrames(const unsigned char* reference,unsigned size,unsigned elapsedMs,void* target) {
    if (size<BackBytes+BackTableBytes) return false;
    const unsigned frame=static_cast<unsigned>(static_cast<uint64_t>(elapsedMs)*BackFps/1000%BackFrames);
    const auto* table=reference+BackBytes;
    const auto* stream=table+BackTableBytes;
    uint32_t bounds[2];
    std::memcpy(bounds,table+frame*sizeof(uint32_t),sizeof bounds);
    const unsigned capacity=size-BackBytes-BackTableBytes;
    if (bounds[0]>=bounds[1] || bounds[1]>capacity) return false;
    const auto* next=stream+bounds[0];
    const auto* end=stream+bounds[1];
    auto* out=static_cast<unsigned char*>(target);
    unsigned remaining=BackBytes;
    while (next<end) {
        const unsigned control=*next++;
        const unsigned bytes=2*((control&127)+1);
        if (bytes>remaining) return false;
        if (control&128) {
            if (end-next<static_cast<int>(bytes)) return false;
            for (unsigned i=0;i<bytes;++i) out[i]=reference[i]^next[i];
            next+=bytes;
        } else {
            std::memcpy(out,reference,bytes);
        }
        reference+=bytes;
        out+=bytes;
        remaining-=bytes;
    }
    return remaining==0;
}

inline bool decodeBack(const void* atlas,unsigned elapsedMs,void* target) {
    const auto* frames=static_cast<const unsigned char*>(atlas)+BackOffset;
    return decodeBackFrames(frames,ButtonBytes-BackOffset,elapsedMs,target);
}
}
