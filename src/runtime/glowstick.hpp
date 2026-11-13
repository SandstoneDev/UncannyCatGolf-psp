#pragma once

#include "visual.hpp"

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <pspkernel.h>
#include <zlib.h>

class GlowstickArt {
public:
    static constexpr unsigned Courses=18, Frames=12, Tracks=Courses+13, TickStep=30, Width=512, Rows=272;
    static constexpr unsigned TrafficIndex=6, DimlitIndex=7, JumpIndex=9, BankIndex=10, HighwayIndex=11, CityIndex=12;
    static constexpr unsigned LightboxIndex=13, AlleyIndex=14;
    static constexpr unsigned DimlitTickStep=25, PoliceTickStep=12, CityTickStep=50;
    static constexpr unsigned SignWidth=512, SignHeight=128;
    static constexpr unsigned CompactInputBytes=8192, CompactInflateLimit=48*1024;
    Patch signs[2][2]{};
    alignas(64) uint32_t signPixels[2][SignWidth*SignHeight]{};

    ~GlowstickArt() { close(); }

    bool open(const char* path) {
        close();
        cached[0]=cached[1]=FrameCount;
        file=std::fopen(path,"rb");
        if (!file || !readHeader()) {
            close();
            return false;
        }
        sceKernelDcacheWritebackRange(signPixels,sizeof signPixels);
        return true;
    }

    const uint16_t* frame(unsigned course,unsigned phase,unsigned signal=0) {
        if (!file || course>=Courses || signal>2) return nullptr;
        unsigned track=course;
        if (signal) {
            switch (course) {
                case TrafficIndex: track=Courses+signal-1; break;
                case DimlitIndex: track=Courses+signal+1; break;
                case JumpIndex: track=Courses+signal+3; break;
                case BankIndex: if (signal!=1) return nullptr; track=Courses+6; break;
                case HighwayIndex: track=Courses+signal+6; break;
                case CityIndex: track=Courses+signal+8; break;
                case LightboxIndex: track=Courses+signal+10; break;
                default: return nullptr;
            }
        }
        phase%=Frames;
        const unsigned index=track*Frames+phase,slot=phase%2;
        const unsigned prior=cached[slot];
        if (prior==index || (compressed && prior<FrameCount && entries[prior].offset==entries[index].offset))
            return pixels[slot];
        cached[slot]=FrameCount;
        if (compressed) {
            if (!readCompressed(entries[index],pixels[slot])) return nullptr;
        } else if (std::fseek(file,Offset+index*FrameBytes,SEEK_SET)!=0 ||
                   std::fread(pixels[slot],FrameBytes,1,file)!=1) {
            return nullptr;
        }
        sceKernelDcacheWritebackRange(pixels[slot],FrameBytes);
        cached[slot]=index;
        return pixels[slot];
    }

private:
    static constexpr uint32_t LegacyMagic=0x33574355, CompactMagic=0x34574355;
    static constexpr unsigned FrameCount=Tracks*Frames, FrameBytes=Width*Rows*2;
    static constexpr unsigned Offset=4+sizeof signs+sizeof signPixels;
    struct Entry { uint32_t offset, bytes; };
    static_assert(sizeof(Entry)==8);
    static constexpr unsigned IndexEnd=Offset+FrameCount*sizeof(Entry);

    std::FILE* file=nullptr;
    bool compressed=false;
    unsigned cached[2]{};
    alignas(64) uint16_t pixels[2][Width*Rows]{};
    std::array<Entry,FrameCount> entries{};
    std::array<unsigned char,CompactInputBytes> input{};

    void close() {
        if (file) std::fclose(file);
        file=nullptr;
        compressed=false;
    }

    bool readHeader() {
        if (std::fseek(file,0,SEEK_END)!=0) return false;
        const long size=std::ftell(file);
        uint32_t magic=0;
        if (size<static_cast<long>(Offset) || static_cast<unsigned long>(size)>UINT32_MAX ||
            std::fseek(file,0,SEEK_SET)!=0 || std::fread(&magic,sizeof magic,1,file)!=1 ||
            (magic!=LegacyMagic && magic!=CompactMagic) ||
            std::fread(signs,sizeof signs,1,file)!=1 ||
            std::fread(signPixels,sizeof signPixels,1,file)!=1) return false;

        for (const auto& language:signs) {
            for (const Patch& sign:language) {
                if (sign.u<0 || sign.v<0 || sign.width<=0 || sign.height<=0 ||
                    sign.u+sign.width>static_cast<int>(SignWidth) || sign.v+sign.height>static_cast<int>(SignHeight) ||
                    sign.x<136 || sign.x+sign.width>408 || sign.y<0 || sign.y+sign.height>272) return false;
            }
        }
        if (magic==LegacyMagic) return size==static_cast<long>(Offset+FrameCount*FrameBytes);
        if (!readIndex(static_cast<uint32_t>(size))) return false;
        compressed=true;
        return true;
    }

    bool readIndex(uint32_t size) {
        if (size<IndexEnd || std::fread(entries.data(),sizeof(Entry),FrameCount,file)!=FrameCount) return false;
        uint32_t end=IndexEnd;
        for (auto it=entries.begin();it!=entries.end();++it) {
            const Entry entry=*it;
            if (entry.offset<IndexEnd || entry.offset>size || !entry.bytes ||
                entry.bytes>FrameBytes+1024 || entry.bytes>size-entry.offset) return false;
            if (entry.offset==end) {
                end+=entry.bytes;
            } else if (std::none_of(entries.begin(),it,[entry](Entry prior) {
                return prior.offset==entry.offset && prior.bytes==entry.bytes;
            })) {
                return false;
            }
        }
        return end==size;
    }

    static voidpf allocate(voidpf opaque,uInt count,uInt size) {
        auto& available=*static_cast<size_t*>(opaque);
        if (!count || size>available/count) return nullptr;
        auto* memory=std::calloc(count,size);
        if (memory) available-=static_cast<size_t>(count)*size;
        return memory;
    }

    static void release(voidpf,voidpf memory) { std::free(memory); }

    bool readCompressed(Entry entry,uint16_t* output) {
        if (std::fseek(file,entry.offset,SEEK_SET)!=0) return false;
        size_t available=CompactInflateLimit;
        z_stream stream{};
        stream.zalloc=allocate;
        stream.zfree=release;
        stream.opaque=&available;
        if (inflateInit(&stream)!=Z_OK) return false;
        stream.next_out=reinterpret_cast<Bytef*>(output);
        stream.avail_out=FrameBytes;
        unsigned remaining=entry.bytes;
        unsigned char overflow=0;
        int result=Z_OK;
        while (result==Z_OK) {
            if (!stream.avail_in && remaining) {
                const unsigned bytes=std::min(remaining,CompactInputBytes);
                if (std::fread(input.data(),1,bytes,file)!=bytes) break;
                stream.next_in=input.data();
                stream.avail_in=bytes;
                remaining-=bytes;
            }
            // The zlib trailer can arrive after the final output byte.
            if (!stream.avail_out) {
                stream.next_out=&overflow;
                stream.avail_out=1;
            }
            result=inflate(&stream,Z_NO_FLUSH);
            if (stream.total_out>FrameBytes) break;
        }
        const bool complete=result==Z_STREAM_END && stream.total_in==entry.bytes &&
            stream.total_out==FrameBytes;
        inflateEnd(&stream);
        return complete;
    }
};
