#pragma once

#include "asset_file.hpp"
#include "visual.hpp"
#include <cstdlib>
#include <malloc.h>
#include <pspkernel.h>
#include <string>

struct ParkFreePixels {
    void operator()(void* data) const { std::free(data); }
};

template<unsigned Width,unsigned Height,unsigned Capacity>
class ParkPatchBank {
public:
    bool load(const std::string& filename,const char** reason=nullptr,
              uint32_t* videoMemory=nullptr,uint32_t* scratch=nullptr) {
        if (texture() && path==filename && borrowed==videoMemory) return true;
        if (reason) *reason="park_block_header";
        const auto file=openAssetFile(filename.c_str());
        uint32_t header[2]{};
        Patch next[Capacity]{};
        if (!file || std::fread(header,sizeof header,1,file.get())!=1 || header[0]!=0x42504355 ||
            !header[1] || header[1]>Capacity ||
            std::fread(next,sizeof(Patch),header[1],file.get())!=header[1]) return false;
        for (unsigned i=0;i<header[1];++i) {
            const auto& p=next[i];
            if (p.u<0 || p.v<0 || p.width<=0 || p.height<=0 ||
                p.u+p.width>static_cast<int>(Width) || p.v+p.height>static_cast<int>(Height)) return false;
        }
        if (videoMemory) pixels.reset();
        else if (!pixels) pixels.reset(static_cast<uint32_t*>(memalign(64,TextureBytes)));
        auto* staging=videoMemory?scratch:pixels.get();
        if (reason) *reason=staging?"park_block_pixels":"park_block_memory";
        if (!staging || std::fread(staging,TextureBytes,1,file.get())!=1 ||
            std::fgetc(file.get())!=EOF || std::ferror(file.get())) {
            unload();
            return false;
        }
        if (videoMemory) {
            auto* uncached=reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(videoMemory)|0x40000000u);
            std::memcpy(uncached,staging,TextureBytes);
        } else sceKernelDcacheWritebackRange(staging,TextureBytes);
        borrowed=videoMemory;
        count=header[1];
        std::copy_n(next,count,art);
        path=filename;
        return true;
    }

    bool loadDirect(const std::string& filename,uint32_t* videoMemory,bool uncached=true) {
        if (texture() && path==filename && borrowed==videoMemory) return true;
        const auto input=openAssetFile(filename.c_str());
        uint32_t header[2]{};
        Patch next[Capacity]{};
        if (!input || !videoMemory || std::fread(header,sizeof header,1,input.get())!=1 ||
            header[0]!=0x42504355 || header[1]!=Capacity ||
            std::fread(next,sizeof(Patch),Capacity,input.get())!=Capacity) return false;
        for (const auto& p:next)
            if (p.u<0 || p.v<0 || p.width<=0 || p.height<=0 ||
                p.u+p.width>static_cast<int>(Width) || p.v+p.height>static_cast<int>(Height)) return false;
        auto* target=reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(videoMemory)|(uncached?0x40000000u:0u));
        alignas(64) uint8_t chunk[4096];
        for (unsigned offset=0;offset<TextureBytes;offset+=sizeof chunk) {
            const unsigned bytes=std::min<unsigned>(sizeof chunk,TextureBytes-offset);
            if (std::fread(chunk,bytes,1,input.get())!=1) return false;
            std::memcpy(target+offset,chunk,bytes);
        }
        if (std::fgetc(input.get())!=EOF || std::ferror(input.get())) return false;
        pixels.reset();
        borrowed=videoMemory;
        count=Capacity;
        if (!uncached) sceKernelDcacheWritebackRange(videoMemory,TextureBytes);
        std::copy_n(next,count,art);
        path=filename;
        return true;
    }

    void unload() {
        pixels.reset();
        borrowed=nullptr;
        path.clear();
        count=0;
    }

    const uint32_t* texture() const { return borrowed?borrowed:pixels.get(); }
    const Patch& patch(unsigned index) const { return art[index]; }
    unsigned size() const { return count; }

private:
    static constexpr unsigned TextureBytes=Width*Height*sizeof(uint32_t);
    std::unique_ptr<uint32_t,ParkFreePixels> pixels;
    uint32_t* borrowed=nullptr;
    Patch art[Capacity]{};
    std::string path;
    unsigned count=0;
};

class ParkSwitchOverlay {
public:
    static constexpr unsigned Side=256, States=2;

    bool load(const std::string& filename) {
        if (path==filename) return true;
        const auto file=openAssetFile(filename.c_str());
        uint32_t header[2]{};
        Patch next[States]{};
        if (!file || std::fread(header,sizeof header,1,file.get())!=1 ||
            header[0]!=0x53544355 || header[1]!=States ||
            std::fread(next,sizeof next,1,file.get())!=1 ||
            std::fseek(file.get(),0,SEEK_END)!=0 ||
            std::ftell(file.get())!=static_cast<long>(DataOffset+States*TextureBytes)) return false;
        for (const auto& p:next) {
            if (p.u || p.v || p.x<0 || p.y<0 || p.width<=0 || p.height<=0 ||
                p.width>static_cast<int>(Side) || p.height>static_cast<int>(Side) ||
                p.x+p.width>272 || p.y+p.height>272) return false;
        }
        std::copy(std::begin(next),std::end(next),art);
        path=filename;
        cached=States;
        return true;
    }

    void unload() {
        pixels.reset();
        path.clear();
        cached=States;
    }

    const uint16_t* frame(unsigned state,Patch& patch) {
        if (state>=States || path.empty()) return nullptr;
        if (cached!=state) {
            if (!pixels) pixels.reset(static_cast<uint16_t*>(memalign(64,TextureBytes)));
            const auto file=openAssetFile(path.c_str());
            cached=States;
            if (!pixels || !file || std::fseek(file.get(),DataOffset+state*TextureBytes,SEEK_SET)!=0 ||
                std::fread(pixels.get(),TextureBytes,1,file.get())!=1) return nullptr;
            sceKernelDcacheWritebackRange(pixels.get(),TextureBytes);
            cached=state;
        }
        patch=art[state];
        return pixels.get();
    }

private:
    static constexpr unsigned TextureBytes=Side*Side*sizeof(uint16_t);
    static constexpr unsigned DataOffset=2*sizeof(uint32_t)+States*sizeof(Patch);
    std::unique_ptr<uint16_t,ParkFreePixels> pixels;
    Patch art[States]{};
    std::string path;
    unsigned cached=States;
};
