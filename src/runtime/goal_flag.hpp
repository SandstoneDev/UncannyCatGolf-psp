#pragma once

#include "asset_file.hpp"
#include "visual.hpp"
#include <cstdio>
#include <pspkernel.h>
#include <string>
#include <zlib.h>

class GoalFlagArt {
public:
    static constexpr unsigned Size=64, SignalCount=3, FieldFrames=12, FlagFrames=4;

    static constexpr unsigned ScratchBytes=SignalCount*FieldFrames*FlagFrames*Size*Size*sizeof(uint16_t);

    uint8_t* scratch() {
        loaded=courses;
        return reinterpret_cast<uint8_t*>(frames);
    }

    bool open(const char* filename,unsigned count) {
        const auto input=openAssetFile(filename);
        auto* file=input.get();
        if (!file || !count) return false;
        uint32_t header[2]{};
        if (std::fread(header,sizeof header,1,file)!=1 || (header[0]!=Raw && header[0]!=Packed) || header[1]!=count) return false;
        const bool compressed=header[0]==Packed;
        if (std::fseek(file,0,SEEK_END)!=0) return false;
        const long bytes=std::ftell(file);
        unsigned offset=sizeof header+count*sizeof(Entry);
        if (bytes<static_cast<long>(offset) || std::fseek(file,sizeof header,SEEK_SET)!=0) return false;
        for (unsigned index=0;index<count;++index) {
            Entry entry{};
            if (std::fread(&entry,sizeof entry,1,file)!=1 ||
                (compressed?entry.offset<offset || entry.offset>static_cast<unsigned>(bytes):entry.offset!=offset)) return false;
            if (compressed) offset=entry.offset;
            const auto& area=entry.area;
            if (!entry.phases && !entry.signals && !area.width && !area.height) continue;
            if ((entry.phases!=1 && entry.phases!=2 && entry.phases!=4 && entry.phases!=FieldFrames) ||
                (entry.signals!=1 && entry.signals!=SignalCount) ||
                area.u!=0 || area.v!=0 || area.width<=0 || area.height<=0 ||
                area.width>static_cast<int>(Size) || area.height>static_cast<int>(Size) ||
                area.x<136 || area.x+area.width>408 || area.y<0 || area.y+area.height>272) return false;
            if (!compressed) offset+=textures(entry)*sizeof frames[0];
        }
        if (!compressed && bytes!=static_cast<long>(offset)) return false;
        packed=compressed;
        fileBytes=static_cast<unsigned>(bytes);
        path=filename;
        courses=count;
        loaded=courses;
        return true;
    }

    bool load(unsigned slot,uint8_t* scratch,unsigned scratchBytes) {
        if (slot>=courses) return false;
        if (slot==loaded) return true;
        loaded=courses;
        const auto input=openAssetFile(path.c_str());
        auto* file=input.get();
        if (!file || std::fseek(file,2*sizeof(uint32_t)+slot*sizeof(Entry),SEEK_SET)!=0 ||
            std::fread(&entry,sizeof entry,1,file)!=1) return false;
        const unsigned count=textures(entry);
        if (count) {
            if (count>SignalCount*FieldFrames*FlagFrames) return false;
            if (packed) {
                unsigned end=fileBytes;
                if (slot+1<courses) {
                    Entry next{};
                    if (std::fread(&next,sizeof next,1,file)!=1) return false;
                    end=next.offset;
                }
                if (entry.offset>=end || end>fileBytes ||
                    !inflatePage(file,entry.offset,end-entry.offset,count*sizeof frames[0],scratch,scratchBytes)) return false;
            } else if (std::fseek(file,entry.offset,SEEK_SET)!=0 ||
                       std::fread(frames,sizeof frames[0],count,file)!=count) return false;
            sceKernelDcacheWritebackRange(frames,count*sizeof frames[0]);
        }
        loaded=slot;
        return true;
    }

    const uint16_t* frame(unsigned slot,unsigned signal,unsigned phase,unsigned tick,Patch& area) const {
        if (slot>=courses || loaded!=slot) return nullptr;
        if (!entry.phases) return nullptr;
        area=entry.area;
        const unsigned state=(signal%entry.signals)*entry.phases+phase%entry.phases;
        return frames[state*FlagFrames+tick/5%FlagFrames];
    }

private:
    static constexpr uint32_t Raw=0x32464355,Packed=0x33464355;
    bool packed=false;
    unsigned fileBytes=0;

    bool inflatePage(std::FILE* file,unsigned offset,unsigned bytes,unsigned expected,
                     uint8_t* scratch,unsigned scratchBytes) {
        if (bytes>scratchBytes || std::fseek(file,offset,SEEK_SET)!=0 ||
            std::fread(scratch,bytes,1,file)!=1) return false;
        // Decode in one pass so Fat does not allocate a separate deflate window.
        unsigned long outputBytes=expected,inputBytes=bytes;
        return uncompress2(reinterpret_cast<Bytef*>(frames),&outputBytes,scratch,&inputBytes)==Z_OK &&
               inputBytes==bytes && outputBytes==expected;
    }

    struct Entry { Patch area; uint32_t phases,signals,offset; };
    static_assert(sizeof(Entry)==24);
    static unsigned textures(const Entry& entry) { return entry.phases*entry.signals*FlagFrames; }
    Entry entry{};
    alignas(64) uint16_t frames[SignalCount*FieldFrames*FlagFrames][Size*Size]{};
    std::string path;
    unsigned courses=0,loaded=0;
};
