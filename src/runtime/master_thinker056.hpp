#pragma once
#include "master_hud_art056.hpp"
namespace MasterThinker056 {
inline void draw() {
    if(Costumes054::selected==VillaCostumes::Id::Canny || !Costumes054::screenActorReady056())return;
    static bool published=false;
    if(!published) {
        sceKernelDcacheWritebackRange(const_cast<uint16_t*>(MasterHudArt056::Palette),sizeof MasterHudArt056::Palette);
        sceKernelDcacheWritebackRange(const_cast<unsigned char*>(MasterHudArt056::Pixels),sizeof MasterHudArt056::Pixels);
        published=true;
    }
    sceGuClutMode(GU_PSM_5650,0,255,0);sceGuClutLoad(16,MasterHudArt056::Palette);
    setTexture(GU_PSM_T8,64,64,MasterHudArt056::Pixels);
    patch({0,0,51,49,0,0},ViewX+83,223);
    Costumes054::drawScreenActor056(false,ViewX+204*17.f/32,468*17.f/32,2.75f);
}
}
