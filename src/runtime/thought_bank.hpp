#pragma once

#include "asset_file.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <zlib.h>

// UCTB stores the original swizzled frames. UCTG stores four-frame groups,
// keeping the logical timing rows and restoring the same 64x64 texture bytes.
class ThoughtBank {
public:
    static constexpr unsigned ClipCount=385, FrameBytes=64*64*2;
    struct Entry { uint32_t first,count,fps,loops; };

    bool open(const char* path) {
        file.reset();
        cached=directoryFirst=0;
        frames=0;
        auto candidate=openAssetFile(path);
        auto* input=candidate.get();
        uint32_t header[3]{};
        if (!input || std::fseek(input,0,SEEK_END)!=0) return false;
        const long size=std::ftell(input);
        if (size<static_cast<long>(TimingEnd) || static_cast<uint64_t>(size)>UINT32_MAX || std::fseek(input,0,SEEK_SET)!=0 ||
            std::fread(header,sizeof header,1,input)!=1 ||
            (header[0]!=LegacyMagic && header[0]!=CompactMagic) ||
            header[1]!=ClipCount || !header[2] || header[2]>32768 ||
            std::fread(entries.data(),sizeof(Entry),entries.size(),input)!=entries.size()) return false;

        const unsigned total=header[2];
        unsigned end=0;
        for (const auto& entry:entries) {
            if (entry.first!=end || !entry.count || entry.count>total-end ||
                !entry.fps || entry.fps>60 || entry.loops>10) return false;
            end+=entry.count;
        }
        if (end!=total) return false;
        const bool grouped=header[0]==CompactMagic;
        const unsigned blocks=(total+FramesPerGroup-1)/FramesPerGroup;
        const unsigned payload=TimingEnd+(grouped?blocks*sizeof(Group):0);
        if (grouped ? size<=static_cast<long>(payload) :
                      size!=static_cast<long>(TimingEnd+total*FrameBytes)) return false;

        compact=grouped;
        frames=total;
        groups=blocks;
        dataStart=payload;
        fileBytes=static_cast<unsigned>(size);
        file=std::move(candidate);
        return true;
    }

    const Entry& entry(unsigned index) const { return entries[index]; }
    unsigned totalFrames() const { return frames; }

    const uint16_t* frame(unsigned source) {
        if (!file || source>=frames) return nullptr;
        if (!compact) {
            if (source+1!=cached) {
                cached=0;
                if (!read(TimingEnd+source*FrameBytes,pixels[0],FrameBytes)) return nullptr;
                cached=source+1;
            }
            return pixels[0];
        }

        const unsigned group=source/FramesPerGroup;
        if (group+1!=cached && !decode(group)) return nullptr;
        return pixels[source%FramesPerGroup];
    }

private:
    static constexpr uint32_t LegacyMagic=0x42544355, CompactMagic=0x47544355;
    static constexpr uint32_t Xor=0x80000000;
    static constexpr unsigned FramesPerGroup=4, MaxCompressed=24576, DirectoryPage=64;
    static constexpr unsigned TimingEnd=12+ClipCount*sizeof(Entry);
    static_assert(sizeof(Entry)==16);

    struct Group { uint32_t offset,spec; };
    static_assert(sizeof(Group)==8);

    bool read(unsigned offset,void* target,unsigned bytes) {
        return std::fseek(file.get(),static_cast<long>(offset),SEEK_SET)==0 &&
               std::fread(target,1,bytes,file.get())==bytes;
    }

    const Group* record(unsigned group) {
        const unsigned start=group-group%DirectoryPage;
        if (start+1!=directoryFirst) {
            directoryFirst=0;
            const unsigned count=std::min(DirectoryPage,groups-start);
            if (!read(TimingEnd+start*sizeof(Group),directory.data(),count*sizeof(Group))) return nullptr;
            directoryFirst=start+1;
        }
        return &directory[group-start];
    }

    bool decode(unsigned group) {
        cached=0;
        const Group* item=record(group);
        if (!item || item->offset<dataStart || item->offset>fileBytes) return false;
        const unsigned bytes=item->spec&~Xor;
        if (!bytes || bytes>fileBytes-item->offset) return false;

        const unsigned count=std::min(FramesPerGroup,frames-group*FramesPerGroup);
        const unsigned expanded=count*FrameBytes;
        if (bytes==expanded && !(item->spec&Xor)) {
            if (!read(item->offset,pixels,expanded)) return false;
        } else {
            if (bytes>=expanded || bytes>sizeof encoded || !read(item->offset,encoded,bytes)) return false;
            uLongf written=expanded;
            uLong consumed=bytes;
            if (uncompress2(reinterpret_cast<Bytef*>(pixels),&written,encoded,&consumed)!=Z_OK ||
                written!=expanded || consumed!=bytes) return false;
            if (item->spec&Xor) {
                for (unsigned i=1;i<count;++i)
                    for (unsigned j=0;j<FrameBytes/2;++j) pixels[i][j]^=pixels[i-1][j];
            }
        }
        cached=group+1;
        return true;
    }

    AssetFile file;
    std::array<Entry,ClipCount> entries{};
    std::array<Group,DirectoryPage> directory{};
    unsigned frames=0,groups=0,dataStart=0,fileBytes=0;
    // Store each cache key plus one so zero marks an empty cache.
    unsigned cached=0,directoryFirst=0;
    bool compact=false;
    alignas(64) uint16_t pixels[FramesPerGroup][FrameBytes/2]{};
    unsigned char encoded[MaxCompressed]{};
};
