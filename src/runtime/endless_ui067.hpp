#pragma once
#include "endless_ui067_data.hpp"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <zlib.h>

// Menu/result only. The storage is the existing camera capture workspace; the
// gameplay capture writer invalidates this lease before touching its pixels.
namespace EndlessUi067 {
inline constexpr uint32_t Green=0xff00ff00u;
inline unsigned char* data=nullptr;
alignas(64) inline uint32_t alpha[256]{};
inline void invalidate(){data=nullptr;}
inline bool prepare(void* workspace,unsigned bytes) {
    if(!workspace || bytes<EndlessUi067Data::RawBytes)return false;
    if(data==workspace)return true;
    uLongf size=EndlessUi067Data::RawBytes;
    if(uncompress(static_cast<Bytef*>(workspace),&size,EndlessUi067Data::Deflate,sizeof EndlessUi067Data::Deflate)!=Z_OK || size!=EndlessUi067Data::RawBytes)return false;
    for(unsigned i=0;i<256;++i)alpha[i]=(i<<24)|0xffffffu;
    sceKernelDcacheWritebackRange(workspace,size);
    sceKernelDcacheWritebackRange(alpha,sizeof alpha);
    __sync_synchronize();
    data=static_cast<unsigned char*>(workspace);
    return true;
}
inline unsigned bank(unsigned requested,unsigned glyph){return russian() || (glyph>=95 && glyph<=160)?2:requested;}
inline unsigned advance(unsigned requested,unsigned glyph){return EndlessUi067Data::Advance[bank(requested,glyph)][glyph];}
inline unsigned width(unsigned font,const char* text){unsigned w=0;while(*text)w+=advance(font,nextGlyph(text));return w;}
inline void text(unsigned font,const char* s,float x,float y,float scale,uint32_t color=Green,bool centered=false,unsigned maxWidth=0,bool keepHeight=false) {
    if(!data)return;
    const float requested=scale;
    if(centered || maxWidth){const unsigned w=width(font,s);if(maxWidth && w)scale=std::min(scale,float(maxWidth)/w);if(centered)x-=w*scale*.5f;}
    sceGuClutMode(GU_PSM_8888,0,255,0);sceGuClutLoad(32,alpha);
    unsigned bound=~0u;
    while(*s){
        const unsigned glyph=nextGlyph(s),b=bank(font,glyph);
        if(bound!=b){setTexture(GU_PSM_T8,512,128,data+EndlessUi067Data::GameoverBytes+b*EndlessUi067Data::FontBytes);sceGuTexMode(GU_PSM_T8,0,0,GU_FALSE);sceGuTexFilter(GU_NEAREST,GU_NEAREST);sceGuTexFlush();bound=b;}
        patchScaled({int16_t(glyph%32*16),int16_t(glyph/32*20),16,20,0,0},ViewX+x,y,scale,keepHeight?requested:scale,color);
        x+=EndlessUi067Data::Advance[b][glyph]*scale;
    }
}
inline void paragraph(const char* s,float x,float y,float scale,unsigned maxWidth,float leading,bool centered=false) {
    if(!data)return;
    while(*s){
        while(*s==' ')++s;
        if(!*s)break;
        const char* first=s;const char* space=nullptr;unsigned w=0,count=0;
        while(*s && *s!='\n'){
            const char* glyph=s;const unsigned a=advance(0,nextGlyph(s));
            if((w+a)*scale>maxWidth && count){s=space?space:glyph;break;}
            if(*glyph==' ')space=glyph;
            w+=a;++count;
        }
        char line[512]{};const auto bytes=std::size_t(s-first);
        if(bytes>=sizeof line)return;
        std::memcpy(line,first,bytes);
        text(0,line,x,y,scale,Green,centered,maxWidth,true);y+=leading;
        if(*s==' ' || *s=='\n')++s;
    }
}
inline void gameOver(float centerX,float centerY) {
    if(!data)return;
    // Fonts and GAME OVER deliberately reload their own complete CLUT on every
    // draw. Neither can inherit the other's palette after a capture generation.
    sceGuClutMode(GU_PSM_8888,0,255,0);
    sceGuClutLoad(32,data+EndlessUi067Data::GameoverIndexBytes);
    setTexture(GU_PSM_T8,512,256,data);
    sceGuTexMode(GU_PSM_T8,0,0,GU_FALSE);sceGuTexFilter(GU_NEAREST,GU_NEAREST);sceGuTexFlush();
    patch({0,0,512,256,0,0},ViewX+centerX-136,centerY-68,17.f/32);
}
}
