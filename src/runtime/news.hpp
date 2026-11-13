#pragma once

#include "visual.hpp"
#include <pspkernel.h>

#include <algorithm>
#include <cstdio>
#include <string>

class NewsArt {
public:
    static constexpr unsigned Lines=16, Width=512, BandHeight=64, LineHeight=32;
    static constexpr unsigned TextHeight=17, TextY=255;
    static constexpr float StartX=425, ClipLeft=136, ClipRight=408, ScrollSpeed=1.328125f;
    Patch band{};
    alignas(64) uint16_t bandPixels[Width*BandHeight]{};

    bool open(const char* path) {
        auto* file=std::fopen(path,"rb");
        if (!file) return false;
        uint32_t magic=0;
        const bool ok=std::fread(&magic,sizeof magic,1,file)==1 && magic==0x574e4355 &&
                      std::fread(&band,sizeof band,1,file)==1 &&
                      std::fread(lines,sizeof lines,1,file)==1 &&
                      std::fread(bandPixels,sizeof bandPixels,1,file)==1 &&
                      std::fseek(file,0,SEEK_END)==0;
        const long fileSize=ok?std::ftell(file):-1;
        std::fclose(file);
        if (fileSize<0 || band.u<0 || band.v<0 || band.width<=0 || band.height<=0 ||
            band.u+band.width>static_cast<int>(Width) || band.v+band.height>static_cast<int>(BandHeight) ||
            band.x<ClipLeft || band.x+band.width>ClipRight || band.y<0 || band.y+band.height>272)
            return false;
        unsigned pages=0;
        for (const auto& language:lines) {
            for (const auto& line:language) {
                if (!line.width || line.width>4096 || line.pages!=(line.width+Width-1)/Width ||
                    line.firstPage!=pages) return false;
                pages+=line.pages;
            }
        }
        if (fileSize!=static_cast<long>(Offset+pages*PageBytes)) return false;
        filename=path;
        sceKernelDcacheWritebackRange(bandPixels,sizeof bandPixels);
        return true;
    }

    unsigned width(bool russian,unsigned line) const { return lines[russian][line].width; }

    const uint32_t* page(bool russian,unsigned line,unsigned number) {
        const auto& headline=lines[russian][line];
        if (number>=headline.pages) return nullptr;
        const unsigned index=headline.firstPage+number,slot=index%2;
        if (cached[slot]==index) return pixels[slot];
        auto* file=std::fopen(filename.c_str(),"rb");
        if (!file) return nullptr;
        cached[slot]=UINT32_MAX;
        const bool ok=std::fseek(file,Offset+index*PageBytes,SEEK_SET)==0 &&
                      std::fread(pixels[slot],PageBytes,1,file)==1;
        std::fclose(file);
        if (!ok) return nullptr;
        sceKernelDcacheWritebackRange(pixels[slot],PageBytes);
        cached[slot]=index;
        return pixels[slot];
    }

private:
    struct Line { uint16_t width,pages; uint32_t firstPage; };
    static_assert(sizeof(Line)==8);
    static constexpr unsigned PageBytes=Width*LineHeight*sizeof(uint32_t);
    static constexpr unsigned Offset=4+sizeof(Patch)+2*Lines*sizeof(Line)+sizeof bandPixels;
    Line lines[2][Lines]{};
    std::string filename;
    unsigned cached[2]{UINT32_MAX,UINT32_MAX};
    alignas(64) uint32_t pixels[2][Width*LineHeight]{};
};

struct NewsRun {
    unsigned headline=0,startedAt=0;

    float x(unsigned tick) const { return NewsArt::StartX-(tick-startedAt)*NewsArt::ScrollSpeed; }

    void reset(unsigned tick) {
        headline=sceKernelGetSystemTimeLow()%NewsArt::Lines;
        startedAt=tick;
    }
};
