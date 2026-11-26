#pragma once
#include "asset_file.hpp"
#include "asset_layout.hpp"
#include "locale.hpp"
#include "release_paths.hpp"
#include <algorithm>
#include <cstdint>
#include <cstdio>

// Main owns selection/persistence. Editor only reads its last active selection
// and the same existing swizzled 5650 pixels from decorations.pak.
namespace EditorSideBackground068 {
constexpr unsigned Width=128,Height=272,PixelBytes=Width*Height*2;
constexpr unsigned Offset=8+BackgroundCount*sizeof(uint32_t)+512*256*2;
constexpr unsigned PackBytes=Offset+BackgroundCount*PixelBytes;
constexpr unsigned Left=36,Right=444;
struct Border { unsigned x,u,width; };
constexpr Border Borders[]={{0,0,Left},{Right,64,480-Right}};
inline unsigned selected(const BackgroundPreference& preference) {
    // Main already rolled and persisted `last` at launch. Do not roll/write
    // again when loadexec enters Editor, including random-background mode.
    return preference.random?preference.last:preference.chosen;
}
struct Image {
    alignas(64) uint16_t pixels[Width*Height]{};
    unsigned active=UINT32_MAX;
    bool ready=false;
    // Call only at initial boot with no queued GE readers. Pixels then remain
    // immutable for this process, independent of EditorArt/ground/OSK caches.
    bool read(std::FILE* file,unsigned id) {
        ready=false;active=UINT32_MAX;
        const unsigned index=backgroundIndex(id);
        uint32_t header[2]{},ids[BackgroundCount]{};
        if(!file||index>=BackgroundCount||std::fseek(file,0,SEEK_END)||
           std::ftell(file)!=long(PackBytes)||std::fseek(file,0,SEEK_SET)||
           std::fread(header,sizeof header,1,file)!=1||header[0]!=0x48474355||header[1]!=BackgroundCount||
           std::fread(ids,sizeof ids,1,file)!=1||
           !std::equal(std::begin(ids),std::end(ids),std::begin(BackgroundIds))||
           std::fseek(file,Offset+index*PixelBytes,SEEK_SET)||
           std::fread(pixels,PixelBytes,1,file)!=1)return false;
        active=id;ready=true;return true;
    }
    bool load(const char* executable) {
        const BackgroundFile setting(ReleasePaths::state(executable,"background.cfg"));
        const auto preference=BackgroundPreference::decode(setting.read(BackgroundPreference{}.encode()));
        const auto path=ReleasePaths::assetDirectory(executable)+AssetLayout::relative("decorations.pak");
        auto input=openAssetFile(path.c_str());
        return read(input.get(),selected(preference));
    }
};
}
