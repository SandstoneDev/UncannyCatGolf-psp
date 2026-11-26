#pragma once
#include "editor_toolbar_plate066.inc"
namespace EditorToolbarPlate066 {
alignas(64) inline uint32_t pixels[64*64]{};
inline bool ready=false;
inline bool draw(){
 if(!ready){uLongf bytes=sizeof pixels;if(uncompress(reinterpret_cast<Bytef*>(pixels),&bytes,Deflate,sizeof Deflate)!=Z_OK||bytes!=sizeof pixels)return false;sceKernelDcacheWritebackRange(pixels,sizeof pixels);sceGuTexFlush();ready=true;}
 setTexture(GU_PSM_8888,64,64,pixels);sceGuTexFilter(GU_NEAREST,GU_NEAREST);
 constexpr float source[]={0,4,60,64},xs[]={256,260,764,768},ys[]={0,4,36,40};
 for(unsigned y=0;y<3;++y)for(unsigned x=0;x<3;++x){
  auto*v=static_cast<TextureVertex*>(sceGuGetMemory(2*sizeof(TextureVertex)));
  v[0]={source[x],source[y],White,ViewX+xs[x]*EditorArt::Scale,ys[y]*EditorArt::Scale,0};
  v[1]={source[x+1],source[y+1],White,ViewX+xs[x+1]*EditorArt::Scale,ys[y+1]*EditorArt::Scale,0};
  sceGuDrawArray(GU_SPRITES,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,2,nullptr,v);
 }
 return true;
}
}
