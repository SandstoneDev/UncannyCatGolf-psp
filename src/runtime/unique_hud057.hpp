#pragma once
#include "unique_hud_art057.hpp"
void drawTv(const uint16_t*,bool);
namespace UniqueHud057 {
alignas(64) inline uint16_t sidebar[MenuScroll::HudBytes/2];
inline const uint16_t* tv=nullptr;
inline bool prepared=false,published=false;
inline unsigned language=2;
// Copy the fixed HUD record before borrowing another clips workspace. This
// keeps Ending's decompression and replay results from invalidating GE inputs.
inline void prepare(Screen screen,unsigned tick) {
    if (screen!=Screen::SoundTest && screen!=Screen::EndlessMenu &&
        screen!=Screen::EndlessHole && screen!=Screen::EndlessResult && screen!=Screen::ElysianEnding && screen!=Screen::Settings) return;
    const unsigned wanted=static_cast<unsigned>(russian());
    if (language!=wanted) {
        language=wanted; // Missing optional art gets one attempt per locale.
        MenuScroll::Textures art{};
        prepared=clips.menu(menuScrollPack.c_str(),2*wanted,art);
        if (prepared) {
            std::memcpy(sidebar,art.hud,sizeof sidebar);
            sceKernelDcacheWritebackRange(sidebar,sizeof sidebar);
        }
    }
    tv=thoughts.menuFrame(tick);
    if (!published) {
        sceKernelDcacheWritebackRange(const_cast<uint32_t*>(UniqueHudArt057::Pixels),sizeof UniqueHudArt057::Pixels);
        sceKernelDcacheWritebackRange(const_cast<uint16_t*>(MasterHudArt056::Palette),sizeof MasterHudArt056::Palette);
        sceKernelDcacheWritebackRange(const_cast<unsigned char*>(MasterHudArt056::Pixels),sizeof MasterHudArt056::Pixels);
        published=true;
    }
}
inline void drawThinker() { MasterThinker056::draw(); }
inline void clearThinker() {
    sceGuClutMode(GU_PSM_5650,0,255,0);sceGuClutLoad(16,MasterHudArt056::Palette);
    setTexture(GU_PSM_T8,64,64,MasterHudArt056::Pixels);
    patch({0,0,51,49,0,0},ViewX+83,223);
}
inline void drawMaster(bool thinker=true) {
    if (prepared) {
        setTexture(GU_PSM_5650,MenuScroll::LargeWidth,512,sidebar);
        hudPatch({0,0,136,272,0,0});
    }
    if (!thinker) clearThinker();
    drawTv(tv,thinker);
}
inline void sprite(unsigned part,float x,float y) {
    const auto& p=UniqueHudArt057::Parts[part];
    setTexture(GU_PSM_8888,256,256,UniqueHudArt057::Pixels);
    patch({static_cast<int16_t>(p.u),static_cast<int16_t>(p.v),static_cast<int16_t>(p.w),static_cast<int16_t>(p.h),0,0},ViewX+x,y);
}
inline void timer(unsigned ticks) {
    // AnimatedSprite2D position192,352;64px frames at20fps, centered.
    sprite(ticks/3%44,(192-32)*17.f/32,(352-32)*17.f/32);
}
inline void demon(float filled,bool glowing) {
    constexpr float X=2*17.f/32,Y=288*17.f/32;
    if(glowing)sprite(47,(2-5)*17.f/32,(288-5)*17.f/32);
    sprite(44,X,Y);
    // Godot FILL_BOTTOM_TO_TOP=3, original22x208 progress rectangle.
    const float bottom=496*17.f/32,top=bottom-208*17.f/32*std::clamp(filled,0.f,1.f);
    sceGuScissor(ViewX,static_cast<int>(top),ViewX+136,static_cast<int>(bottom)+1);
    sprite(45,X,Y);
    sceGuScissor(0,0,Width,Height);
    sprite(46,X,Y);
}
}
