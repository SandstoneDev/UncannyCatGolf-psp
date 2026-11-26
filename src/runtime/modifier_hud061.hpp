#pragma once
#include "modifier_hud_art061.hpp"
#include "modifier_hud_state062.hpp"

namespace ModifierHud061 {
inline State state;
inline bool published=false;
inline bool framePresented=false;
inline void draw(const State& value,uint64_t now) {
    if(value.context==Context::None || !value.mask)return;
    if(!published) {
        sceKernelDcacheWritebackRange(ModifierHudArt061::Palette,sizeof ModifierHudArt061::Palette);
        sceKernelDcacheWritebackRange(ModifierHudArt061::Pixels,sizeof ModifierHudArt061::Pixels);
        published=true;
    }
    sceGuScissor(ViewX,0,ViewX+408,Height);
    sceGuClutMode(GU_PSM_8888,0,15,0);sceGuClutLoad(2,ModifierHudArt061::Palette);
    setTexture(GU_PSM_T4,128,128,ModifierHudArt061::Pixels);
    forEach(value,now,ViewX,[](unsigned i,float x,float y,float scale){
        patchScaled({static_cast<int16_t>(i%4*32),static_cast<int16_t>(i/4*32),32,32,0,0},x,y,scale,scale,White);
    });
    sceGuScissor(0,0,Width,Height);
}
inline void present(uint64_t now) {
    if(framePresented)return;
    draw(state,now);
    framePresented=true;
}
} // namespace ModifierHud061
