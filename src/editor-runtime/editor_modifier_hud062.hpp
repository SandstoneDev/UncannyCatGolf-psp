#pragma once
#include "modifier_hud_state062.hpp"
#include "modifier_hud_art061.hpp"
namespace EditorModifierHud062 {
inline ModifierHud061::State state;
inline bool published=false,framePresented=false;
inline void present(uint64_t now){
    if(framePresented)return;
    framePresented=true;
    if(state.context==ModifierHud061::Context::None || !state.mask)return;
    if(!published){
        sceKernelDcacheWritebackRange(ModifierHudArt061::Palette,sizeof ModifierHudArt061::Palette);
        sceKernelDcacheWritebackRange(ModifierHudArt061::Pixels,sizeof ModifierHudArt061::Pixels);
        published=true;
    }
    const int saved[]={EditorArt::uiScissor[0],EditorArt::uiScissor[1],EditorArt::uiScissor[2],EditorArt::uiScissor[3]};
    EditorArt::scissor(ViewX,0,ViewX+408,ScreenHeight);
    sceGuClutMode(GU_PSM_8888,0,15,0);sceGuClutLoad(2,ModifierHudArt061::Palette);
    setTexture(GU_PSM_T4,128,128,ModifierHudArt061::Pixels);
    ModifierHud061::forEach(state,now,ViewX,[](unsigned i,float x,float y,float scale){
        auto* v=static_cast<TextureVertex*>(sceGuGetMemory(2*sizeof(TextureVertex)));
        v[0]={float(i%4*32),float(i/4*32),White,x,y,0};
        v[1]={float(i%4*32+32),float(i/4*32+32),White,x+32*scale,y+32*scale,0};
        sceGuDrawArray(GU_SPRITES,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,2,nullptr,v);
    });
    EditorArt::scissor(saved[0],saved[1],saved[2],saved[3]);
}
} // namespace EditorModifierHud062
