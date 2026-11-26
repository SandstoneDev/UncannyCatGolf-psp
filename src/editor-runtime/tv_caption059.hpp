#pragma once
#include "tv_caption059_data.hpp"
namespace TvCaption059 {
inline bool published=false;
inline void publish(){
 if(published)return;
 sceKernelDcacheWritebackRange(const_cast<uint16_t*>(Data::Cleanup),sizeof Data::Cleanup);
 published=true;
}
// Owner-requested caption removal. Restore only the original tiny ink area
// with adjacent pixels from the same source TV bezel. No new label or panel.
inline void draw(){
 setTexture(GU_PSM_5650,128,8,Data::Cleanup);sceGuTexFilter(GU_NEAREST,GU_NEAREST);
 auto*v=static_cast<TextureVertex*>(sceGuGetMemory(2*sizeof(TextureVertex)));
 v[0]={0,0,0xffffffffu,float(ViewX+18),179,0};
 v[1]={67,4,0xffffffffu,float(ViewX+85),183,0};
 sceGuDrawArray(GU_SPRITES,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,2,nullptr,v);
}
}
