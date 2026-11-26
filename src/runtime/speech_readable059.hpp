#pragma once
#include "avatar_speech058_layout.hpp"
#include "avatar_speech_projection059.hpp"
#include "speech_placement059.hpp"
namespace SpeechReadable059 {
inline bool published=false;
inline void publish(){
 if(published)return;
 sceKernelDcacheWritebackRange(const_cast<uint8_t*>(AvatarSpeech058::Data::Atlas),sizeof AvatarSpeech058::Data::Atlas);
 sceKernelDcacheWritebackRange(const_cast<uint32_t*>(AvatarSpeech058::Data::Palette),sizeof AvatarSpeech058::Data::Palette);
 published=true;
}
inline void draw(float x,float y,const AvatarSpeech058::Layout& layout,uint32_t tint){
 if(!layout.lineCount||!(tint>>24))return;
 unsigned widest=0;for(unsigned i=0;i<layout.lineCount;++i)widest=std::max(widest,unsigned(layout.lines[i].width));
 const auto p=SpeechPlacement059::place(x,y,layout.lineCount,widest);if(!p.valid)return;
 sceGuDisable(GU_TEXTURE_2D);
 auto* body=static_cast<ColorVertex*>(sceGuGetMemory(2*sizeof(ColorVertex)));
 body[0]={tint,p.left,p.top,0};body[1]={tint,p.right,p.bottom,0};
 sceGuDrawArray(GU_SPRITES,GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,2,nullptr,body);
 auto* tail=static_cast<ColorVertex*>(sceGuGetMemory(3*sizeof(ColorVertex)));
 for(unsigned i=0;i<3;++i)tail[i]={tint,p.tail[i].x,p.tail[i].y,0};
 sceGuDrawArray(GU_TRIANGLE_FAN,GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,3,nullptr,tail);
 sceGuClutMode(GU_PSM_8888,0,255,0);sceGuClutLoad(32,AvatarSpeech058::Data::Palette);
 setTexture(GU_PSM_T8,AvatarSpeech058::Data::Width,AvatarSpeech058::Data::Height,AvatarSpeech058::Data::Atlas);
 sceGuTexFilter(GU_NEAREST,GU_NEAREST);
 const AvatarProjection059::Projection clip{p.anchorX,p.anchorY,p.scale,p.scale,p.left,p.right,p.top,p.bottom};
 for(unsigned i=0;i<layout.glyphCount;++i){
  const auto& g=layout.glyphs[i];const auto& image=AvatarSpeech058::Data::Images[g.image];
  AvatarProjection059::Quad q;
  if(!clip.quad(g.x+image.dx,g.y+image.dy,g.x+image.dx+image.width,g.y+image.dy+image.height,image.u,image.v,image.u+image.width,image.v+image.height,q))continue;
  auto* v=static_cast<TextureVertex*>(sceGuGetMemory(2*sizeof(TextureVertex)));
  v[0]={q.u0,q.v0,tint,q.x0,q.y0,0};v[1]={q.u1,q.v1,tint,q.x1,q.y1,0};
  sceGuDrawArray(GU_SPRITES,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,2,nullptr,v);
 }
}
}
