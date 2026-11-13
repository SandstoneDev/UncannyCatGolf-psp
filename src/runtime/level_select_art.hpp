#pragma once

#include "level_select.hpp"
#include "visual.hpp"
#include "asset_file.hpp"

#include <cstddef>
#include <string>

#include <cstdio>
#include <pspkernel.h>

class LevelSelectArt {
public:
    static constexpr unsigned Worlds=LevelSelectState::WorldCount, Cards=18, States=13;
    static constexpr unsigned Size=512, Rows=272;
    static constexpr unsigned Locked=6, FramesPerLanguage=Worlds*(Worlds+1)/2;
    static constexpr unsigned FirstFocused=7;

    bool open(const char* filename) {
        const auto input=openAssetFile(filename);
        auto* file=input.get();
        uint32_t magic=0;
        if (!file || std::fread(&magic,sizeof magic,1,file)!=1 || (magic!=0x324c4355 && magic!=0x334c4355) ||
            std::fread(cards,sizeof cards,1,file)!=1 ||
            std::fseek(file,0,SEEK_END)!=0 ||
            std::ftell(file)!=static_cast<long>(FramesOffset+2*FramesPerLanguage*FrameBytes)) return false;
        for (unsigned w=0;w<Worlds;++w) {
            for (unsigned i=0;i<LevelSelectState::cells(w);++i) {
                for (const Patch& p:cards[w][i]) {
                    if (p.u<0 || p.v<0 || p.width<=0 || p.height<=0 || p.u+p.width>static_cast<int>(Size) ||
                        p.v+p.height>static_cast<int>(Size) || p.x<136 || p.y<0 ||
                        p.x+p.width>408 || p.y+p.height>static_cast<int>(Rows)) return false;
                }
            }
        }
        cleanBack=magic==0x334c4355;
        path=filename;
        baseTag=0;
        for (auto& atlas:atlases) atlas.tag=0;
        return true;
    }

    struct CardAtlas {
        alignas(64) uint16_t normal[Size*Size]{};
        alignas(64) uint16_t focus[Size*Rows]{};
        unsigned tag=0;
    };

    static constexpr unsigned VictoryWorkspaceBytes=Size*(Size+Rows)*sizeof(uint16_t);
    uint8_t* victoryWorkspace() {
        static_assert(offsetof(CardAtlas,tag)==VictoryWorkspaceBytes);
        atlases[0].tag=0;
        return reinterpret_cast<uint8_t*>(&atlases[0]);
    }

    uint8_t* victoryPixels() {
        atlases[1].tag=0;
        return reinterpret_cast<uint8_t*>(atlases[1].normal);
    }

    const CardAtlas* loadCards(unsigned world,unsigned keep=LevelSelectState::NoCourse) {
        if (world>=Worlds) return nullptr;
        for (const auto& atlas:atlases) {
            if (atlas.tag==world+1) return &atlas;
        }
        auto& atlas=atlases[keep<Worlds && atlases[0].tag==keep+1 ? 1 : 0];
        atlas.tag=0;
        const auto input=openAssetFile(path.c_str());
        auto* file=input.get();
        if (!file || std::fseek(file,AtlasOffset+world*AtlasBytes,SEEK_SET)!=0 ||
            std::fread(atlas.normal,AtlasBytes,1,file)!=1 ||
            std::fseek(file,FocusOffset+world*FrameBytes,SEEK_SET)!=0 ||
            std::fread(atlas.focus,FrameBytes,1,file)!=1) return nullptr;
        sceKernelDcacheWritebackRange(atlas.normal,AtlasBytes);
        sceKernelDcacheWritebackRange(atlas.focus,FrameBytes);
        atlas.tag=world+1;
        return &atlas;
    }

    static unsigned panelIndex(unsigned world,unsigned latest,bool russian) {
        return static_cast<unsigned>(russian)*FramesPerLanguage+latest*(latest+1)/2+world;
    }

    bool hasCleanBack() const { return cleanBack; }

    void invalidateBase() { baseTag=0; }

    bool loadBase(unsigned world,unsigned latest,bool russian) {
        if (world>=Worlds || latest>=Worlds || world>latest) return false;
        const unsigned frame=panelIndex(world,latest,russian);
        if (baseTag!=frame+1) {
            baseTag=0;
            const auto input=openAssetFile(path.c_str());
            auto* file=input.get();
            if (!file || std::fseek(file,FramesOffset+frame*FrameBytes,SEEK_SET)!=0 ||
                std::fread(base,FrameBytes,1,file)!=1) return false;
            sceKernelDcacheWritebackRange(base,FrameBytes);
            baseTag=frame+1;
        }
        return true;
    }

    bool openGo(const char* filename) {
        const auto input=openAssetFile(filename);
        auto* file=input.get();
        if (!file || std::fseek(file,0,SEEK_END)!=0 ||
            std::ftell(file)!=static_cast<long>(2*FramesPerLanguage*sizeof goPixels)) return false;
        goPath=filename;
        goTag=0;
        return true;
    }

    const uint16_t* goFrame(unsigned world,unsigned latest,bool russian) {
        if (latest>=Worlds || world>latest) return nullptr;
        const auto index=panelIndex(world,latest,russian);
        if (goTag!=index+1) {
            goTag=0;
            const auto input=openAssetFile(goPath.c_str());
            auto* file=input.get();
            if (!file || std::fseek(file,index*sizeof goPixels,SEEK_SET)!=0 ||
                std::fread(goPixels,sizeof goPixels,1,file)!=1) return nullptr;
            sceKernelDcacheWritebackRange(goPixels,sizeof goPixels);
            goTag=index+1;
        }
        return goPixels;
    }

    Patch cards[Worlds][Cards][States]{};
    alignas(64) uint16_t base[Size*Rows]{};

private:
    static constexpr unsigned AtlasOffset=4+sizeof cards;
    static constexpr unsigned AtlasBytes=Size*Size*2, FrameBytes=Size*Rows*2;
    static constexpr unsigned FocusOffset=AtlasOffset+Worlds*AtlasBytes;
    static constexpr unsigned FramesOffset=FocusOffset+Worlds*FrameBytes;
    std::string path,goPath;
    CardAtlas atlases[2]{};
    alignas(64) uint16_t goPixels[64*64]{};
    unsigned baseTag=0,goTag=0;
    bool cleanBack=false;
};
