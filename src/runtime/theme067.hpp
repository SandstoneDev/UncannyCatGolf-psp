#pragma once
#include "theme067_data.hpp"
namespace Theme067 {
alignas(64) inline uint32_t pixels[128*64]{};
inline bool ready=false;
inline bool prepare(){
    if(ready)return true;
    uLongf bytes=sizeof pixels;
    if(uncompress(reinterpret_cast<Bytef*>(pixels),&bytes,Theme067Data::Deflate,sizeof Theme067Data::Deflate)!=Z_OK || bytes!=sizeof pixels)return false;
    sceKernelDcacheWritebackRange(pixels,sizeof pixels);ready=true;return true;
}
inline bool bind(){if(!prepare())return false;setTexture(GU_PSM_8888,128,64,pixels);sceGuTexMode(GU_PSM_8888,0,0,GU_FALSE);sceGuTexFilter(GU_NEAREST,GU_NEAREST);sceGuTexFlush();return true;}
inline void piece(int u,int v,int w,int h,float x,float y,float width,float height,uint32_t tint=White){
    if(width<=0 || height<=0)return;
    patchScaled({int16_t(u),int16_t(v),int16_t(w),int16_t(h),0,0},ViewX+x,y,width/w,height/h,tint);
}
inline void modifier(float x,float y,bool checked,bool focus,bool locked){
    if(!bind())return;
    const int base=locked?16:focus?8:0;
    constexpr float size=12,edge=2.4f;
    const float dx[]={0,edge,size-edge,size};
    // Source margins are four pixels on its 8x8 image. Extend the edge
    // boundary texel through the empty center, preserving the corner size.
    const int uv[]={0,3,4},sz[]={4,1,4};
    for(unsigned row=0;row<3;++row)for(unsigned col=0;col<3;++col)
        piece(base+uv[col],uv[row],sz[col],sz[row],x+dx[col],y+dx[row],dx[col+1]-dx[col],dx[row+1]-dx[row]);
    piece(locked?48:checked?24:36,0,12,12,x+2.4f,y+2.4f,7.2f,7.2f);
}
inline void slider(float x,float y,float width,float fraction,bool focus){
    if(!bind())return;
    fraction=std::clamp(fraction,0.f,1.f);
    constexpr float edge=4.25f,height=8.5f;
    auto track=[&](float w,uint32_t tint){
        const float cap=std::min(edge,w*.5f);
        piece(0,16,8,16,x,y,cap,height,tint);
        piece(8,16,8,16,x+cap,y,w-2*cap,height,tint);
        piece(16,16,8,16,x+w-cap,y,cap,height,tint);
    };
    track(width,0xffabababu);track(width*fraction,focus?White:0xffabababu);
    piece(focus?40:24,16,16,16,x+fraction*width-4.25f,y,8.5f,8.5f);
}
inline void scroll(float x,float y,float height,float fraction,float visible){
    if(!bind())return;
    // PSP's reflow needs a scroll indicator absent from the PC layout. Reuse
    // the actual settings slider texture and rank grabber, with no new colors.
    piece(0,16,24,16,x,y,3,height,0xffabababu);
    const float thumb=std::clamp(height*visible,8.5f,height);
    piece(24,16,16,16,x-1,y+(height-thumb)*fraction,5,thumb);
}
inline void pause(bool ru,float centerX,float y){
    if(!bind())return;
    const int w=ru?80:48;
    piece(0,ru?48:32,w,16,centerX-w*.5f,y,float(w),16,0xff00ff00u);
}
}
