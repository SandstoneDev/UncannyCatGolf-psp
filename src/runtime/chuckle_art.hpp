#pragma once

#include "asset_file.hpp"
#include "visual.hpp"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <new>
#include <utility>

#ifdef __PSP__
#include <pspkernel.h>
#endif

class ChuckleArt {
public:
    static constexpr unsigned Width=256, Height=128, FrameSize=32, Frames=25;

    static constexpr unsigned TextureBytes=Width*Height*sizeof(uint32_t);

    static constexpr Patch frame(unsigned tick) {
        const unsigned index=(tick/2)%Frames;
        const unsigned columns=Width/FrameSize;
        return {static_cast<int16_t>((index%columns)*FrameSize),static_cast<int16_t>((index/columns)*FrameSize),
                FrameSize,FrameSize,-16,-16};
    }

    bool load(const char* path) {
        unload();
        const auto input=openAssetFile(path);
        auto* file=input.get();
        char magic[4]{};
        if (!file || std::fread(magic,sizeof magic,1,file)!=1 ||
            std::memcmp(magic,"UCC1",sizeof magic)!=0) return false;

        std::unique_ptr<Atlas> loaded(new (std::nothrow) Atlas);
        if (!loaded || std::fread(loaded->pixels,sizeof loaded->pixels,1,file)!=1 ||
            std::fgetc(file)!=EOF || std::ferror(file)) return false;
#ifdef __PSP__
        sceKernelDcacheWritebackRange(loaded->pixels,sizeof loaded->pixels);
#endif
        atlas=std::move(loaded);
        return true;
    }

    bool loadBorrowed(const char* path,void* videoMemory) {
        unload();
        if (!videoMemory) return false;
        const auto input=openAssetFile(path);
        auto* file=input.get();
        char magic[4]{};
        if (!file || std::fread(magic,sizeof magic,1,file)!=1 ||
            std::memcmp(magic,"UCC1",sizeof magic)!=0) return false;
        auto* target=static_cast<uint8_t*>(videoMemory);
#ifdef __PSP__
        target=reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(target)|0x40000000u);
#endif
        alignas(64) uint8_t chunk[4096];
        for (unsigned offset=0;offset<TextureBytes;) {
            const unsigned bytes=std::min<unsigned>(sizeof chunk,TextureBytes-offset);
            if (std::fread(chunk,1,bytes,file)!=bytes) return false;
            std::memcpy(target+offset,chunk,bytes);
            offset+=bytes;
        }
        if (std::fgetc(file)!=EOF || std::ferror(file)) return false;
        atlas.reset();
        sharedFile.reset();
        sharedPixels=static_cast<const uint8_t*>(videoMemory);
        borrowed_=true;
        return true;
    }

    bool borrowed() const { return borrowed_; }

    bool openShared(const char* path) {
        if (sharedFile) return true;
        unload();
        auto input=openAssetFile(path);
        char magic[4]{};
        if (!input || std::fread(magic,sizeof magic,1,input.get())!=1 ||
            std::memcmp(magic,"UCC1",sizeof magic)!=0 ||
            std::fseek(input.get(),0,SEEK_END)!=0 ||
            std::ftell(input.get())!=static_cast<long>(sizeof magic+TextureBytes)) return false;
        sharedFile=std::move(input);
        return true;
    }

    bool useShared(uint8_t* workspace,unsigned generation) {
        if (sharedPixels==workspace && sharedGeneration==generation) return true;
        if (!sharedFile || std::fseek(sharedFile.get(),4,SEEK_SET)!=0 ||
            std::fread(workspace,TextureBytes,1,sharedFile.get())!=1) return false;
#ifdef __PSP__
        sceKernelDcacheWritebackRange(workspace,TextureBytes);
#endif
        sharedPixels=workspace;
        sharedGeneration=generation;
        return true;
    }

    void unload() {
        atlas.reset();
        sharedFile.reset();
        sharedPixels=nullptr;
        borrowed_=false;
    }
    const uint32_t* pixels() const {
        return atlas?atlas->pixels:reinterpret_cast<const uint32_t*>(sharedPixels);
    }

private:
    struct alignas(64) Atlas { uint32_t pixels[Width*Height]; };
    std::unique_ptr<Atlas> atlas;
    AssetFile sharedFile;
    const uint8_t* sharedPixels=nullptr;
    unsigned sharedGeneration=0;
    bool borrowed_=false;
};

