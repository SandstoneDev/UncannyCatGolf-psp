#pragma once

#include "villa_costumes054.hpp"
#include "cat_artist_copycat.hpp"
#include "costume_shader056.hpp"
#include "costume_screen056.hpp"
#include "arle_randomizer056.hpp"

namespace Costumes054 {

using Id=VillaCostumes::Id;
namespace Art=VillaCostumes::Art;
using SelectionFile=PreferenceFile<0x43564355,VillaCostumes::validPreference>;
std::string actorPath,residentPath,preferencePath;
extern std::string screenPath056;
Id selected=Id::Canny;
std::unique_ptr<uint32_t[]> actorPixels;

void setPaths(const char* executable) {
    actorPath=assetPath(executable,"villa-costume-actors054.bin");
    residentPath=assetPath(executable,"villa-costume-residents054.bin");
    preferencePath=statePath(executable,"costume.cfg");
    screenPath056=assetPath(executable,"costume-screen056.bin");
}

bool decode(const std::string& path,void* pixels,unsigned size) {
    const auto file=openAssetFile(path.c_str());
    if (!file || std::fseek(file.get(),0,SEEK_END)!=0) return false;
    const long bytes=std::ftell(file.get());
    if (bytes<=0 || static_cast<unsigned long>(bytes)>sizeof fallbackPage ||
        std::fseek(file.get(),0,SEEK_SET)!=0 ||
        std::fread(fallbackPage,static_cast<unsigned>(bytes),1,file.get())!=1) return false;
    fallbackPageIndex=ScreenPages;
    horseMeter039.invalidateShared();
    unsigned long written=size,consumed=static_cast<unsigned long>(bytes);
    return uncompress2(static_cast<uint8_t*>(pixels),&written,
                       reinterpret_cast<const uint8_t*>(fallbackPage),&consumed)==Z_OK &&
           written==size && consumed==static_cast<unsigned long>(bytes);
}

uint32_t* actorPage() {
    return reinterpret_cast<uint32_t*>((reinterpret_cast<uintptr_t>(actorPixels.get())+63u)&~uintptr_t(63u));
}

bool loadBank() {
    if (actorPixels) return true;
    actorPixels.reset(new(std::nothrow) uint32_t[Art::ActorsBytes/4+16]);
    if (!actorPixels || !decode(actorPath,actorPage(),Art::ActorsBytes)) {
        actorPixels.reset();
        return false;
    }
    sceKernelDcacheWritebackRange(actorPage(),Art::ActorsBytes);
    return true;
}

void restore(bool loadArt=true) {
    const auto saved=static_cast<Id>(SelectionFile(preferencePath).read());
    selected=(!loadArt || saved==Id::Canny || loadBank())?saved:Id::Canny;
}

bool select(Id id,VillaUnlocks::Flags flags) {
    if (!VillaCostumes::available(id,flags) || (id!=Id::Canny && !loadBank()) ||
        !SelectionFile(preferencePath).write(static_cast<uint32_t>(id))) return false;
    selected=id;
    if (selected==Id::Canny) actorPixels.reset();
    return true;
}

bool gameplayArt() { return selected!=Id::Canny && actorPixels; }

void prepareGameplay() {
    if (selected!=Id::Canny && !loadBank()) selected=Id::Canny;
}

void bindActorTexture() {
    if (gameplayArt()) setTexture(GU_PSM_8888,Art::ActorsWidth,Art::ActorsHeight,actorPage());
    else setTexture(GU_PSM_8888,ActorsWidth,ActorsHeight,actors);
}

const Patch& patchFor(bool ghost) {
    if (!gameplayArt()) return ghost?visual.ghost:visual.cat;
    const auto pair=selected==Id::Copycat?ArtistCopycat::costume():VillaCostumes::actorPair(selected);
    return Art::Actors[ghost?pair.ghost:pair.player];
}

#include "costume_screen_draw056.inc"

void drawActor(const GameplayView& view,bool ghost,Vec2 world,float scale,uint32_t tint,float angle=0) {
    const auto& image=patchFor(ghost);
    if (image.width>0) {
        if (selected==Id::Rainbow) drawRainbowActor056(view,image,world,scale,tint,angle);
        else if (angle!=0) actorRotated(view,image,world,scale,tint,angle);
        else actor(view,image,world,scale,tint,true);
    }
    if (!ghost && selected==Id::Copycat && ArtistCopycat::drawn) {
        ArtistInk::bind(ArtistCopycat::pixels.data());
        constexpr float inkScale=4*0.064f*GameplayView::RasterScale;
        if (angle!=0) actorRotated(view,ArtistInk::Full,world,scale*inkScale,tint,angle);
        else actor(view,ArtistInk::Full,world,scale*inkScale,tint,true);
        bindActorTexture();
    }
}

}

