#pragma once

#include "asset_file.hpp"
#include "visual.hpp"
#include <cstdio>
#include <pspkernel.h>
#include <string>

class ActorAtlas {
public:
    bool open(const char* filename,unsigned count) {
        const auto input=openAssetFile(filename);
        auto* file=input.get();
        uint32_t header[2]{};
        if (!file || std::fread(header,sizeof header,1,file)!=1 || header[0]!=0x31414355 ||
            header[1]!=count || std::fseek(file,0,SEEK_END)!=0 ||
            std::ftell(file)!=static_cast<long>(sizeof header+count*RecordBytes)) return false;
        path=filename;
        courses=count;
        cached=courses;
        return true;
    }

    bool load(unsigned index,VisualHeader& visual,uint32_t (&pixels)[ActorsWidth*ActorsHeight]) {
        if (index>=courses) return false;
        if (index!=cached) {
            const auto input=openAssetFile(path.c_str());
            auto* file=input.get();
            cached=courses;
            Patch patches[2]{};
            if (!file || std::fseek(file,8+index*RecordBytes,SEEK_SET)!=0 ||
                std::fread(patches,sizeof patches,1,file)!=1 ||
                !valid(patches[0]) || !valid(patches[1]) ||
                std::fread(pixels,sizeof pixels,1,file)!=1) return false;
            cat=patches[0];
            ghost=patches[1];
            cached=index;
            sceKernelDcacheWritebackRange(pixels,sizeof pixels);
        }
        visual.cat=cat;
        visual.ghost=ghost;
        return true;
    }

private:
    static constexpr unsigned RecordBytes=2*sizeof(Patch)+ActorsWidth*ActorsHeight*sizeof(uint32_t);
    static bool valid(const Patch& patch) {
        return patch.u>=0 && patch.v>=0 && patch.width>0 && patch.height>0 &&
               patch.u+patch.width<=ActorsWidth && patch.v+patch.height<=ActorsHeight;
    }
    std::string path;
    Patch cat{},ghost{};
    unsigned courses=0,cached=0;
};
