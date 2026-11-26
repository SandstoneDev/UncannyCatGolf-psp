#pragma once

#include "asset_file.hpp"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <new>
#include <utility>
#include <zlib.h>

namespace MeowlsPresentation {

constexpr unsigned NarrationMs=1000, BlackoutMs=496833, CameoMs=504125, DurationMs=505000;
constexpr unsigned Width=272, Height=272, Stride=512;
constexpr unsigned TilesAcross=Width/8, TilesDown=Height/8, Tiles=TilesAcross*TilesDown;
constexpr unsigned TileBytes=8*8*2, LineBytes=(Stride/8)*TileBytes;
constexpr unsigned TextureBytes=Stride*Height*2, RawMax=Tiles*(TileBytes+4);
constexpr unsigned CameoWidth=64, CameoHeight=128, CameoBytes=CameoWidth*CameoHeight*4;

constexpr unsigned VideoToggle24[]={
    533,629,970,1146,1762,1830,1861,2023,2589,2665,2755,2921,
    3211,3322,3458,3544,4403,4534,4807,4912,5476,5559,6162,6388,
    8279,8487,8968,9137,10353,10485,11417,11564
};

constexpr unsigned videoTick(unsigned elapsedMs) {
    const unsigned now=std::min(elapsedMs,DurationMs)*24;
    unsigned elapsed=0;
    for (unsigned i=0;i<sizeof(VideoToggle24)/sizeof(*VideoToggle24);i+=2) {
        const unsigned start=VideoToggle24[i]*1000;
        const unsigned end=VideoToggle24[i+1]*1000;
        if (start>=now) break;
        elapsed+=std::min(now,end)-start;
    }
    return elapsed*30/24000;
}

class Stream {
public:
    bool open(const char* path,uint8_t* texture,uint8_t* raw,uint8_t* cameo) {
        close();
        auto input=openAssetFile(path);
        Header header{};
        if (!input || std::fread(&header,sizeof header,1,input.get())!=1 ||
            header.magic!=0x3150564du || !header.count || header.count>2790 ||
            !header.cameoLength || header.cameoLength>compressBound(CameoBytes) || header.reserved)
            return false;
        std::unique_ptr<Record[]> table(new(std::nothrow) Record[header.count]);
        if (!table || std::fread(table.get(),sizeof(Record),header.count,input.get())!=header.count ||
            std::fseek(input.get(),0,SEEK_END)!=0)
            return false;
        const long size=std::ftell(input.get());
        if (size<0 || header.cameoOffset>static_cast<unsigned long>(size) ||
            header.cameoLength!=static_cast<unsigned long>(size)-header.cameoOffset)
            return false;
        uint32_t end=sizeof(Header)+header.count*sizeof(Record);
        uint32_t largest=header.cameoLength;
        for (unsigned i=0;i<header.count;++i) {
            const auto& row=table[i];
            const unsigned rawBytes=row.raw&~Xor;
            if (row.offset!=end || end>header.cameoOffset || !row.length ||
                row.length>header.cameoOffset-end || rawBytes>RawMax ||
                row.length>compressBound(rawBytes) || row.key>i || row.tick>=2790 ||
                (!i && row.tick!=0) || (i && row.tick<=table[i-1].tick))
                return false;
            if (row.key==i) {
                if ((row.raw&Xor) || rawBytes!=Tiles*TileBytes+4) return false;
            } else if (row.key!=table[i-1].key) {
                return false;
            }
            largest=std::max(largest,row.length);
            end+=row.length;
        }
        if (end!=header.cameoOffset) return false;
        std::unique_ptr<uint8_t[]> packed(new(std::nothrow) uint8_t[largest]);
        if (!packed || !read(input.get(),header.cameoOffset,packed.get(),header.cameoLength))
            return false;
        unsigned long bytes=CameoBytes;
        if (uncompress(cameo,&bytes,packed.get(),header.cameoLength)!=Z_OK || bytes!=CameoBytes)
            return false;
        source=std::move(input);
        index=std::move(table);
        compressed=std::move(packed);
        count=header.count;
        frameTexture=texture;
        decoded=raw;
        return select(0);
    }

    void close() {
        source.reset();
        index.reset();
        compressed.reset();
        current=Unset;
        count=0;
        frameTexture=nullptr;
        decoded=nullptr;
    }

    [[nodiscard]] unsigned frameIndex(unsigned tick) const {
        if (!count) return 0;
        const auto* first=index.get();
        return static_cast<unsigned>(std::upper_bound(first,first+count,tick,
                    [](unsigned t,const Record& row) { return t<row.tick; }) -first-1);
    }

    bool select(unsigned frame) {
        if (!source || frame>=count) return false;
        if (current==frame) return true;
        const unsigned key=index[frame].key;
        const unsigned start=current!=Unset && current>=key && current<frame?current+1:key;
        for (unsigned i=start;i<=frame;++i) {
            if (!decode(i)) {
                current=Unset;
                return false;
            }
        }
        current=frame;
        return true;
    }

private:
    static constexpr unsigned Xor=1u<<31, Unset=~0u;
    struct Header { uint32_t magic,count,cameoOffset,cameoLength,reserved; };
    struct Record { uint32_t tick,offset,length,raw,key; };
    static_assert(sizeof(Header)==20 && sizeof(Record)==20);

    AssetFile source;
    std::unique_ptr<Record[]> index;
    std::unique_ptr<uint8_t[]> compressed;
    uint8_t* frameTexture=nullptr;
    uint8_t* decoded=nullptr;
    unsigned count=0,current=Unset;

    static bool read(FILE* file,unsigned offset,uint8_t* out,unsigned size) {
        return std::fseek(file,offset,SEEK_SET)==0 && std::fread(out,size,1,file)==1;
    }

    static unsigned le16(const uint8_t* p) {
        return p[0]+p[1]*256u;
    }

    bool decode(unsigned frame) {
        const auto& row=index[frame];
        if (!read(source.get(),row.offset,compressed.get(),row.length)) return false;
        const bool useXor=(row.raw&Xor)!=0;
        unsigned long bytes=RawMax;
        if (uncompress(decoded,&bytes,compressed.get(),row.length)!=Z_OK || bytes!=(row.raw&~Xor))
            return false;
        auto* cursor=decoded;
        const auto* boundary=decoded+bytes;
        unsigned previous=0;
        while (cursor<boundary) {
            if (boundary-cursor<4) return false;
            unsigned tile=le16(cursor);
            unsigned remaining=le16(cursor+2);
            cursor+=4;
            if (!remaining || tile<previous || tile>=Tiles || remaining>Tiles-tile ||
                static_cast<unsigned>(boundary-cursor)<remaining*TileBytes ||
                (row.key==frame && (tile || remaining!=Tiles)))
                return false;
            previous=tile+remaining;
            while (remaining) {
                const unsigned col=tile%TilesAcross,raster=tile/TilesAcross;
                const unsigned span=std::min(remaining,TilesAcross-col);
                auto* target=frameTexture+raster*LineBytes+col*TileBytes;
                const unsigned size=span*TileBytes;
                if (!useXor) std::memcpy(target,cursor,size);
                else for (unsigned i=0;i<size;++i) target[i]^=cursor[i];
                cursor+=size;
                tile+=span;
                remaining-=span;
            }
        }
        return true;
    }
};

#ifndef UCG_MEOWLS_PRESENTATION_HEADLESS
constexpr unsigned TextureOffset=0x88000, CameoOffset=TextureOffset+TextureBytes;
constexpr unsigned RawOffset=CameoOffset+CameoBytes;
static_assert(RawOffset+RawMax<2*1024*1024);
inline Stream film;
inline unsigned loadedFrame=~0u;

inline void close() {
    film.close();
    loadedFrame=~0u;
}

inline bool open(const char* executable) {
    close();
    const auto path=companion(executable,"data/world/meowls-presentation.bin");
    if (!film.open(path.c_str(),vram045(TextureOffset),vram045(RawOffset),vram045(CameoOffset))) {
        close();
        return false;
    }
    sceKernelDcacheWritebackRange(vram045(CameoOffset),CameoBytes);
    return true;
}

// Call inside a GE list after the previous frame has finished.
inline bool draw(unsigned elapsedMs) {
    constexpr unsigned X=ViewX+136;
    if (elapsedMs>=BlackoutMs) {
        rectangle(X,0,Width,Height,0xff000000u);
        if (elapsedMs>=CameoMs) {
            sceGuTexFlush();
            setTexture(GU_PSM_8888,CameoWidth,CameoHeight,vram045(CameoOffset));
            patch({0,0,64,98,0,0},ViewX+161,174,1.0f);
        }
        return true;
    }

    const unsigned frame=film.frameIndex(videoTick(elapsedMs));
    if (frame!=loadedFrame) {
        if (!film.select(frame)) return false;
        sceKernelDcacheWritebackRange(vram045(TextureOffset),TextureBytes);
        loadedFrame=frame;
        sceGuTexFlush();
    }
    const unsigned level=elapsedMs>=2500?255:elapsedMs*255/2500;
    const uint32_t tint=0xff000000u | (level<<16) | (level<<8) | level;
    setTexture(GU_PSM_5650,Stride,512,vram045(TextureOffset));
    patch({0,0,272,272,0,0},X,0,1.0f,tint);
    return true;
}
#endif

} // namespace MeowlsPresentation
