#pragma once

#include "visual.hpp"

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <pspkernel.h>
#include <zlib.h>

class GlowstickArt {
public:
    static constexpr unsigned Courses=18, Frames=12, Tracks=Courses+13, TickStep=30, Width=512, Rows=272;
    static constexpr unsigned RushHourIndex=5, RushHourWidth=272;
    static constexpr unsigned RushHourVramBytes=(Frames-2)*RushHourWidth*Rows*2;
    static constexpr unsigned TrafficIndex=6, DimlitIndex=7, JumpIndex=9, BankIndex=10, HighwayIndex=11, CityIndex=12;
    static constexpr unsigned DimlitVramBytes=2*1024*1024-2*Width*Rows*2;
    static constexpr unsigned DimlitBlockBytes=64, DimlitBlocks=RushHourWidth*Rows*2/DimlitBlockBytes;
    static constexpr unsigned DimlitMapBytes=3*Frames*DimlitBlocks*sizeof(uint16_t);
    static constexpr unsigned DimlitHashBytes=65536*sizeof(uint16_t);
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

    void discardPreparedCourse() {
        if (!rushHour && !dimlitVram && !frozenCourse) return;
        rushHour=nullptr;
        dimlitVram=nullptr;
        frozenCourse=0;
        cached[0]=cached[1]=FrameCount;
    }

    unsigned diskFrames() const { return diskFrameCount; }

    bool preparedFor(unsigned course) const {
        return (course==RushHourIndex && rushHour) || (course==DimlitIndex && dimlitVram);
    }

    void prepareDimlit(uint16_t* vram,uint16_t* workspace,unsigned workspaceBytes) {
        if (dimlitVram || !file || !vram || !workspace || workspaceBytes<DimlitMapBytes) return;
        discardPreparedCourse();
        constexpr unsigned blockWords=DimlitBlockBytes/sizeof(uint16_t);
        constexpr unsigned hashCount=DimlitHashBytes/sizeof(uint16_t);
        constexpr unsigned vramBlocks=DimlitVramBytes/DimlitBlockBytes;
        static_assert(DimlitVramBytes%DimlitBlockBytes==0 && DimlitHashBytes<=FrameBytes);
        const unsigned capacity=vramBlocks+(workspaceBytes-DimlitMapBytes)/DimlitBlockBytes;
        auto* uncached=reinterpret_cast<uint16_t*>(reinterpret_cast<uintptr_t>(vram)|0x40000000u);
        auto* spill=workspace+DimlitMapBytes/sizeof(uint16_t);
        const auto blockAt=[&](unsigned index) {
            return index<vramBlocks?uncached+index*blockWords:spill+(index-vramBlocks)*blockWords;
        };
        auto* hash=pixels[1];
        std::fill_n(hash,hashCount,UINT16_MAX);
        cached[0]=cached[1]=FrameCount;
        unsigned used=0;
        auto* map=workspace;
        for (unsigned signal=0;signal<3;++signal) {
            const unsigned track=signal?Courses+signal+1:DimlitIndex;
            for (unsigned phase=0;phase<Frames;++phase) {
                if (!loadFrame(track*Frames+phase,pixels[0])) return;
                for (unsigned band=0;band<Rows/8;++band) {
                    const auto* row=pixels[0]+band*Width*8;
                    for (unsigned col=0;col<RushHourWidth*8;col+=blockWords) {
                        const auto* block=row+col;
                        unsigned code=2166136261u;
                        for (unsigned i=0;i<blockWords;++i) code=(code^block[i])*16777619u;
                        unsigned bucket=(code^(code>>16))&(hashCount-1);
                        while (hash[bucket]!=UINT16_MAX &&
                               std::memcmp(block,blockAt(hash[bucket]),DimlitBlockBytes)!=0)
                            bucket=(bucket+1)&(hashCount-1);
                        if (hash[bucket]==UINT16_MAX) {
                            if (used==capacity || used==UINT16_MAX) return;
                            std::memcpy(blockAt(used),block,DimlitBlockBytes);
                            hash[bucket]=used++;
                        }
                        *map++=hash[bucket];
                    }
                }
            }
        }
        dimlitMap=workspace;
        dimlitVram=uncached;
    }

    // The round stops ticking on a live result. Preserve its last blend in RAM.
    void freeze(unsigned course,unsigned phase,unsigned signal) {
        if (!preparedFor(course)) return;
        phase%=Frames;
        const unsigned next=(phase+1)%Frames;
        const auto* first=frame(course,phase,signal);
        const auto* second=frame(course,next,signal);
        if (!first || !second) { discardPreparedCourse(); return; }
        if (course==RushHourIndex) {
            if (first!=pixels[phase%2]) std::memcpy(pixels[phase%2],first,RushHourBytes);
            if (second!=pixels[next%2]) std::memcpy(pixels[next%2],second,RushHourBytes);
            sceKernelDcacheWritebackRange(pixels,sizeof pixels);
        }
        cached[phase%2]=signal*Frames+phase;
        cached[next%2]=signal*Frames+next;
        rushHour=nullptr;
        dimlitVram=nullptr;
        frozenCourse=course+1;
    }

    void prepareCourse(unsigned course,uint16_t* vram) {
        if (course!=RushHourIndex) {
            discardPreparedCourse();
            return;
        }
        if (rushHour || !file || !vram) return;
        discardPreparedCourse();

        // An eight-row swizzle band consists of 16-byte-wide blocks. Keep the visible blocks.
        constexpr unsigned sourceBand=Width*2*8, visibleBand=RushHourWidth*2*8;
        constexpr unsigned vramFrames=Frames-2;
        auto* targetVram=reinterpret_cast<unsigned char*>(reinterpret_cast<uintptr_t>(vram)|0x40000000u);
        cached[0]=cached[1]=FrameCount;
        for (unsigned phase=0;phase<Frames;++phase) {
            auto* scratch=pixels[phase<vramFrames?0:phase-vramFrames];
            if (!loadFrame(course*Frames+phase,scratch)) return;
            auto* source=reinterpret_cast<unsigned char*>(scratch);
            auto* target=phase<vramFrames?targetVram+phase*RushHourBytes:source;
            for (unsigned band=0;band<Rows/8;++band) {
                std::memmove(target,source+band*sourceBand,visibleBand);
                target+=visibleBand;
            }
        }
        sceKernelDcacheWritebackRange(pixels,sizeof pixels);
        rushHour=vram;
    }

    unsigned textureStride(unsigned course,unsigned signal) const {
        return course+1==frozenCourse || (course==DimlitIndex && dimlitVram) || (course==RushHourIndex && !signal && rushHour)
            ? RushHourWidth : Width;
    }

    const uint16_t* frame(unsigned course,unsigned phase,unsigned signal=0) {
        if (!file || course>=Courses || signal>2) return nullptr;
        phase%=Frames;
        if (course+1==frozenCourse && cached[phase%2]==signal*Frames+phase)
            return pixels[phase%2];
        if (course==DimlitIndex && dimlitVram) {
            constexpr unsigned blockWords=DimlitBlockBytes/sizeof(uint16_t);
            constexpr unsigned vramBlocks=DimlitVramBytes/DimlitBlockBytes;
            const unsigned key=signal*Frames+phase,slot=phase%2;
            if (cached[slot]!=key) {
                const auto* map=dimlitMap+key*DimlitBlocks;
                const auto* spill=dimlitMap+DimlitMapBytes/sizeof(uint16_t);
                for (unsigned i=0;i<DimlitBlocks;++i) {
                    const unsigned block=map[i];
                    const auto* source=block<vramBlocks?dimlitVram+block*blockWords:spill+(block-vramBlocks)*blockWords;
                    std::memcpy(pixels[slot]+i*blockWords,source,DimlitBlockBytes);
                }
                sceKernelDcacheWritebackRange(pixels[slot],RushHourBytes);
                cached[slot]=key;
            }
            return pixels[slot];
        }
        if (course==RushHourIndex && !signal && rushHour) {
            return phase<Frames-2?rushHour+phase*(RushHourBytes/2):pixels[phase-(Frames-2)];
        }
        discardPreparedCourse();
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
        const unsigned index=track*Frames+phase,slot=phase%2;
        const unsigned prior=cached[slot];
        if (prior==index || (compressed && prior<FrameCount && entries[prior].offset==entries[index].offset))
            return pixels[slot];
        cached[slot]=FrameCount;
        ++diskFrameCount;
        if (!loadFrame(index,pixels[slot])) return nullptr;
        sceKernelDcacheWritebackRange(pixels[slot],FrameBytes);
        cached[slot]=index;
        return pixels[slot];
    }

private:
    static constexpr uint32_t LegacyMagic=0x33574355, CompactMagic=0x34574355;
    static constexpr unsigned FrameCount=Tracks*Frames, FrameBytes=Width*Rows*2;
    static constexpr unsigned RushHourBytes=RushHourWidth*Rows*2;
    static_assert(RushHourWidth%8==0 && RushHourBytes%64==0);
    static constexpr unsigned Offset=4+sizeof signs+sizeof signPixels;
    struct Entry { uint32_t offset, bytes; };
    static_assert(sizeof(Entry)==8);
    static constexpr unsigned IndexEnd=Offset+FrameCount*sizeof(Entry);

    std::FILE* file=nullptr;
    uint16_t* rushHour=nullptr;
    const uint16_t* dimlitVram=nullptr;
    const uint16_t* dimlitMap=nullptr;
    unsigned frozenCourse=0;
    bool compressed=false;
    unsigned diskFrameCount=0;
    unsigned cached[2]{};
    alignas(64) uint16_t pixels[2][Width*Rows]{};
    std::array<Entry,FrameCount> entries{};
    std::array<unsigned char,CompactInputBytes> input{};

    void close() {
        discardPreparedCourse();
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

    bool loadFrame(unsigned index,uint16_t* output) {
        if (compressed) return readCompressed(entries[index],output);
        return std::fseek(file,Offset+index*FrameBytes,SEEK_SET)==0 && std::fread(output,FrameBytes,1,file)==1;
    }

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

