#pragma once

#include "asset_file.hpp"
#include "park_stage_art.hpp"

#include <array>
#include <cstdint>
#include <malloc.h>
#include <pspkernel.h>
#include <string>
#include <zlib.h>

class RideArt033 {
public:
    static constexpr unsigned Width=512, TrackHeight=256, BackgroundHeight=128;

    bool load(const std::string& trackPath,const std::string& backgroundPath) {
        if (trackFile && backgroundFile) return true;
        auto trackInput=openAssetFile(trackPath.c_str());
        auto backgroundInput=openAssetFile(backgroundPath.c_str());
        if (!trackInput || !backgroundInput || std::fseek(trackInput.get(),0,SEEK_END)!=0 ||
            std::ftell(trackInput.get())!=static_cast<long>(2*TrackBytes) ||
            std::fseek(backgroundInput.get(),0,SEEK_END)!=0) return false;
        const long size=std::ftell(backgroundInput.get());
        uint32_t header[7]{};
        if (size<0 || std::fseek(backgroundInput.get(),0,SEEK_SET)!=0 ||
            std::fread(header,sizeof header,1,backgroundInput.get())!=1 ||
            header[0]!=0x47424355 || header[1]!=Width || header[2]!=BackgroundHeight ||
            header[3]!=LoopFrames || header[4]!=ExitFrames || header[5]!=LoopStep || header[6]!=ExitStep ||
            std::fread(entries.data(),sizeof(Entry),entries.size(),backgroundInput.get())!=entries.size())
            return false;
        uint32_t expected=sizeof header+sizeof entries;
        for (const auto& frame:entries) {
            if (frame.offset!=expected || !frame.bytes || frame.bytes>BackgroundBytes+1024 ||
                frame.offset>static_cast<uint32_t>(size) || frame.bytes>static_cast<uint32_t>(size)-frame.offset)
                return false;
            expected+=frame.bytes;
        }
        if (expected!=static_cast<uint32_t>(size)) return false;
        trackFile=std::move(trackInput);
        backgroundFile=std::move(backgroundInput);
        return true;
    }

    static constexpr unsigned TrackBytes=Width*TrackHeight*2;
    static constexpr unsigned BackgroundBytes=Width*BackgroundHeight*2;
    static constexpr unsigned CompressedBytes=BackgroundBytes+1024;

    void useWorkspace(uint16_t* trackMemory,unsigned char* backgroundMemory,unsigned generation) {
        trackPixels=trackMemory;
        backgroundPixels=reinterpret_cast<uint16_t*>(backgroundMemory);
        compressed=backgroundMemory+BackgroundBytes;
        if (generation!=workspaceGeneration) {
            workspaceGeneration=generation;
            trackIndex=2;
            backgroundIndex=LoopFrames+ExitFrames;
        }
    }

    void unload() {
        trackFile.reset();
        backgroundFile.reset();
        trackIndex=2;
        backgroundIndex=LoopFrames+ExitFrames;
    }

    const uint16_t* track(bool rails) {
        if (!trackFile || !trackPixels) return nullptr;
        const unsigned index=rails;
        if (index!=trackIndex) {
            trackIndex=2;
            if (std::fseek(trackFile.get(),index*TrackBytes,SEEK_SET)!=0 ||
                std::fread(trackPixels,TrackBytes,1,trackFile.get())!=1) return nullptr;
            sceKernelDcacheWritebackRange(trackPixels,TrackBytes);
            trackIndex=index;
        }
        return trackPixels;
    }

    const uint16_t* background(bool exiting,float progress) {
        if (!backgroundFile || !backgroundPixels) return nullptr;
        const unsigned position=static_cast<unsigned>(std::max(0.0f,progress)/(exiting?ExitStep:LoopStep)+0.5f);
        const unsigned index=exiting ? LoopFrames+std::min(ExitFrames-1,position) : position%LoopFrames;
        if (index!=backgroundIndex) {
            backgroundIndex=LoopFrames+ExitFrames;
            const auto entry=entries[index];
            if (std::fseek(backgroundFile.get(),entry.offset,SEEK_SET)!=0 ||
                std::fread(compressed,entry.bytes,1,backgroundFile.get())!=1) return nullptr;
            unsigned long bytes=BackgroundBytes;
            if (uncompress(reinterpret_cast<Bytef*>(backgroundPixels),&bytes,compressed,entry.bytes)!=Z_OK ||
                bytes!=BackgroundBytes) return nullptr;
            sceKernelDcacheWritebackRange(backgroundPixels,BackgroundBytes);
            backgroundIndex=index;
        }
        return backgroundPixels;
    }

private:
    static constexpr unsigned LoopFrames=580, ExitFrames=458, LoopStep=1, ExitStep=4;
    struct Entry { uint32_t offset,bytes; };
    std::array<Entry,LoopFrames+ExitFrames> entries{};
    AssetFile trackFile,backgroundFile;
    uint16_t* trackPixels=nullptr;
    uint16_t* backgroundPixels=nullptr;
    unsigned char* compressed=nullptr;
    unsigned trackIndex=2,backgroundIndex=LoopFrames+ExitFrames,workspaceGeneration=0;
};
template<unsigned Width,unsigned Height,unsigned Count>
class RideAtlas033 {
public:
    static constexpr unsigned TextureWidth=Width, TextureHeight=Height;

    bool load(const std::string& path) {
        if (pixels) return true;
        const auto file=openAssetFile(path.c_str());
        if (!file || !readHeader(file.get())) return false;
        pixels.reset(static_cast<uint8_t*>(memalign(64,TextureBytes)));
        if (!pixels || std::fread(pixels.get(),TextureBytes,1,file.get())!=1 ||
            std::fgetc(file.get())!=EOF || std::ferror(file.get())) {
            unload();
            return false;
        }
        sceKernelDcacheWritebackRange(pixels.get(),TextureBytes);
        return true;
    }

    bool openShared(const std::string& path) {
        if (sharedFile) return true;
        sharedFile=openSharedAtlas(path);
        return static_cast<bool>(sharedFile);
    }

    bool openOnDemand(const std::string& path) {
        if (!openSharedAtlas(path)) return false;
        sharedPath=path;
        return true;
    }

    bool useShared(uint8_t* workspace,unsigned generation) {
        if (sharedPixels==workspace && sharedGeneration==generation) return true;
        sharedPixels=nullptr;
        AssetFile temporary;
        if (!sharedFile && !sharedPath.empty()) temporary=openAssetFile(sharedPath.c_str());
        auto* file=sharedFile?sharedFile.get():temporary.get();
        if (!file || std::fseek(file,PayloadOffset,SEEK_SET)!=0 ||
            std::fread(workspace,TextureBytes,1,file)!=1) return false;
        sceKernelDcacheWritebackRange(workspace,TextureBytes);
        sharedPixels=workspace;
        sharedGeneration=generation;
        return true;
    }

    void unload() {
        pixels.reset();
        sharedFile.reset();
        sharedPath.clear();
        sharedPixels=nullptr;
    }
    void invalidateShared() { sharedPixels=nullptr; }
    const uint8_t* texture() const { return pixels?pixels.get():sharedPixels; }
    const uint32_t* clut() const { return palette; }
    const Patch& patch(unsigned index) const { return patches[index]; }

private:
    static constexpr unsigned TextureBytes=Width*Height;
    static constexpr unsigned PayloadOffset=8+Count*sizeof(Patch)+256*sizeof(uint32_t);

    AssetFile openSharedAtlas(const std::string& path) {
        auto file=openAssetFile(path.c_str());
        if (!file || !readHeader(file.get()) || std::fseek(file.get(),0,SEEK_END)!=0 ||
            std::ftell(file.get())!=static_cast<long>(PayloadOffset+TextureBytes))
            return {};
        return file;
    }

    bool readHeader(std::FILE* file) {
        uint32_t header[2]{};
        if (std::fread(header,sizeof header,1,file)!=1 || header[0]!=0x38504355 || header[1]!=Count ||
            std::fread(patches,sizeof patches,1,file)!=1 ||
            std::fread(palette,sizeof palette,1,file)!=1) return false;
        for (const auto& patch:patches) {
            if (patch.u<0 || patch.v<0 || patch.width<=0 || patch.height<=0 ||
                patch.u+patch.width>static_cast<int>(Width) || patch.v+patch.height>static_cast<int>(Height))
                return false;
        }
        sceKernelDcacheWritebackRange(palette,sizeof palette);
        return true;
    }

    alignas(64) uint32_t palette[256]{};
    Patch patches[Count]{};
    std::unique_ptr<uint8_t,ParkFreePixels> pixels;
    AssetFile sharedFile;
    std::string sharedPath;
    uint8_t* sharedPixels=nullptr;
    unsigned sharedGeneration=0;
};
