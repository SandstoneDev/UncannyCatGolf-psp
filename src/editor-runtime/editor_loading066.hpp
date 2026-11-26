#pragma once
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <zlib.h>
#include "chapter_loading066.inc"
namespace EditorLoading066 {
inline constexpr unsigned Stride=512,Height=272,FramePixels=Stride*Height,IconSize=64,Left=208,Top=104;
alignas(64) inline uint16_t frames[10][IconSize*IconSize]{};
inline bool ready=false;
inline bool prepare(void*scratch,unsigned bytes){
 if(ready)return true;
 constexpr unsigned raw=10*32*32*4;if(!scratch||bytes<raw)return false;
 uLongf size=raw;
 if(uncompress(static_cast<Bytef*>(scratch),&size,ChapterLoading066::PrismDeflate,sizeof ChapterLoading066::PrismDeflate)!=Z_OK||size!=raw)return false;
 const auto*rgba=static_cast<const uint8_t*>(scratch);
 for(unsigned f=0;f<10;++f)for(unsigned y=0;y<64;++y)for(unsigned x=0;x<64;++x){
  const auto*p=rgba+4*(f*32*32+(y/2)*32+x/2);const unsigned a=p[3];
  const unsigned r=p[0]*a/255,g=p[1]*a/255,b=p[2]*a/255;
  frames[f][y*64+x]=uint16_t((r>>3)|((g>>2)<<5)|((b>>3)<<11));
 }
 ready=true;return true;
}
inline bool draw(uint16_t*video,unsigned bytes,unsigned frame,unsigned buffer){
 if(!ready||!video||bytes<FramePixels*4||frame>=10||buffer>=2)return false;
 auto*out=video+buffer*FramePixels+Top*Stride+Left;
 for(unsigned y=0;y<64;++y)std::memcpy(out+y*Stride,frames[frame]+y*64,64*2);
 return true;
}
inline bool paint(uint16_t*video,unsigned bytes,uint16_t*scratch,unsigned scratchBytes){
 if(!video||bytes<FramePixels*4||!prepare(scratch,scratchBytes))return false;
 std::fill_n(video,FramePixels*2,uint16_t(0));
 return draw(video,bytes,0,0)&&draw(video,bytes,0,1);
}
}
