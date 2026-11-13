#pragma once

#include "asset_file.hpp"
#include <zlib.h>

class ElysianEnding045 {
public:
    static constexpr unsigned Frames=3174,Fps=20,Side=272,KeyStep=16;
    static constexpr unsigned NativeBytes=Side*Side*2,MaxPayload=112*1024,Stride=512,FrameBytes=Stride*Side*2;

    bool open(const std::string& en,const std::string& ru) {
        paths[0]=en;
        paths[1]=ru;
        return valid(paths[0]) && (en==ru || valid(paths[1]));
    }
    void close() {
        file.reset();
        language=2;
        anchor=shown=Frames;
    }

    const uint16_t* frame(bool russian,unsigned index,uint16_t* out,uint8_t* key,uint8_t* scratch) {
        if (language!=russian) {
            close();
            file=openAssetFile(paths[russian].c_str());
            if (!file) return nullptr;
            language=russian;
        }
        index=std::min(index,Frames-1);
        if (shown==index) return out;
        const unsigned base=index&~(KeyStep-1);
        Record entry{};
        if (!record(file.get(),index,entry)) return nullptr;
        if (entry.bytes&DeltaFlag) {
            if (anchor!=base) {
                Record keyEntry{};
                if (!record(file.get(),base,keyEntry) || !decode(keyEntry,key,scratch+NativeBytes)) return nullptr;
                anchor=base;
            }
            if (!decode(entry,scratch,scratch+NativeBytes)) return nullptr;
            for (unsigned i=0;i<NativeBytes/4;++i)
                reinterpret_cast<uint32_t*>(scratch)[i]^=reinterpret_cast<const uint32_t*>(key)[i];
        } else {
            if (!decode(entry,scratch,scratch+NativeBytes)) return nullptr;
            if (index==base) {
                std::memcpy(key,scratch,NativeBytes);
                anchor=base;
            }
        }
        auto* bytes=reinterpret_cast<uint8_t*>(out);
        constexpr unsigned nativeRow=Side*2*8,gpuRow=Stride*2*8;
        for (unsigned row=0;row<Side/8;++row)
            std::memcpy(bytes+row*gpuRow,scratch+row*nativeRow,nativeRow);
        sceKernelDcacheWritebackRange(out,FrameBytes);
        shown=index;
        return out;
    }

private:
    static constexpr uint32_t DeltaFlag=1u<<31;
    static constexpr unsigned DataStart=24+8*Frames;
    struct Record { uint32_t offset,bytes; };
    AssetFile file;
    std::string paths[2];
    unsigned language=2,anchor=Frames,shown=Frames;

    static bool record(std::FILE* file,unsigned index,Record& entry) {
        return std::fseek(file,24+8*index,SEEK_SET)==0 && std::fread(&entry,sizeof entry,1,file)==1;
    }
    static bool valid(const std::string& path) {
        const auto input=openAssetFile(path.c_str());
        auto* f=input.get();
        uint32_t header[6]{};
        if (!f || std::fseek(f,0,SEEK_END)!=0) return false;
        const long length=std::ftell(f);
        if (length<static_cast<long>(DataStart) || std::fseek(f,0,SEEK_SET)!=0 ||
            std::fread(header,sizeof header,1,f)!=1 ||
            header[0]!=0x31454355u || header[1]!=Frames || header[2]!=Fps ||
            header[3]!=Side || header[4]!=Side || header[5]!=KeyStep) return false;
        unsigned end=DataStart;
        for (unsigned i=0;i<Frames;++i) {
            Record entry{};
            if (std::fread(&entry,sizeof entry,1,f)!=1) return false;
            const unsigned bytes=entry.bytes&~DeltaFlag;
            if ((! (i%KeyStep) && (entry.bytes&DeltaFlag)) || entry.offset!=end ||
                !bytes || bytes>MaxPayload || bytes>static_cast<unsigned>(length)-end) return false;
            end+=bytes;
        }
        return end==static_cast<unsigned>(length);
    }
    bool decode(Record entry,uint8_t* out,uint8_t* packed) {
        const unsigned bytes=entry.bytes&~DeltaFlag;
        if (bytes>MaxPayload || std::fseek(file.get(),entry.offset,SEEK_SET)!=0 ||
            std::fread(packed,bytes,1,file.get())!=1) return false;
        unsigned long length=NativeBytes;
        return uncompress(out,&length,packed,bytes)==Z_OK && length==NativeBytes;
    }
};

struct EndingClock045 {
    static constexpr unsigned DurationMs=158700,MusicStartMs=500,ScaryMs=157500;
    uint64_t micros=0;
    bool music=false,scary=false,fast=false;
    unsigned ms() const { return static_cast<unsigned>(micros/1000); }
};
