#pragma once

#include "park_stage_art.hpp"

class ParkDogScreen {
public:
    static constexpr unsigned Width=512, Height=256, Frames=2;

    bool load(const std::string& filename) {
        if (texture && path==filename) return true;
        const auto file=openAssetFile(filename.c_str());
        uint32_t header[2]{};
        Patch next[Frames]{};
        if (!file || std::fread(header,sizeof header,1,file.get())!=1 ||
            header[0]!=0x34444355 || header[1]!=Frames ||
            std::fread(next,sizeof next,1,file.get())!=1 ||
            std::fread(palette,sizeof palette,1,file.get())!=1) return false;
        for (const auto& p:next) {
            if (p.u<0 || p.v<0 || p.width<=0 || p.height<=0 ||
                p.u+p.width>static_cast<int>(Width) || p.v+p.height>static_cast<int>(Height)) return false;
        }
        if (!texture) texture.reset(static_cast<uint8_t*>(memalign(64,TextureBytes)));
        if (!texture || std::fread(texture.get(),TextureBytes,1,file.get())!=1 ||
            std::fgetc(file.get())!=EOF || std::ferror(file.get())) {
            unload();
            return false;
        }
        std::copy(std::begin(next),std::end(next),art);
        sceKernelDcacheWritebackRange(texture.get(),TextureBytes);
        sceKernelDcacheWritebackRange(palette,sizeof palette);
        path=filename;
        return true;
    }

    void unload() {
        texture.reset();
        path.clear();
    }

    const uint8_t* pixels() const { return texture.get(); }
    const uint32_t* colors() const { return palette; }
    const Patch& patch(unsigned frame) const { return art[frame]; }

private:
    static constexpr unsigned TextureBytes=Width*Height/2;
    std::unique_ptr<uint8_t,ParkFreePixels> texture;
    std::string path;
    Patch art[Frames]{};
    alignas(64) uint32_t palette[16]{};
};
