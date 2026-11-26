#pragma once
#include "endless_presentation.hpp"
#include "endless_rules_text.hpp"
#include "endless_ui067.hpp"

namespace EndlessPresentationArt {
inline unsigned char* pixels=nullptr;
alignas(64) inline uint32_t palette[16];
inline int loaded=-1;
inline uint16_t* capture=nullptr;
constexpr unsigned PixelBytes=512*1024/2,CaptureBytes=512*512*2;
static_assert(EndlessUi067Data::RawBytes<=CaptureBytes,"Endless menu UI must fit the existing camera capture workspace");
inline void borrowWorkspace() {
    EndlessUi067::invalidate();
    static_assert(PixelBytes+CaptureBytes<=LevelSelectArt::VictoryWorkspaceBytes);
    pixels=levelSelectArt.cardWorkspace(1);
    capture=reinterpret_cast<uint16_t*>(pixels+PixelBytes);
    loaded=-1;
    sceKernelDcacheWritebackInvalidateRange(pixels,PixelBytes+CaptureBytes);
}
inline void releaseWorkspace() {EndlessUi067::invalidate();loaded=-1;pixels=nullptr;capture=nullptr;}

inline bool load(const char* executable,unsigned wanted) {
    if (!pixels) borrowWorkspace();
    if (loaded==static_cast<int>(wanted)) return true;
    auto* file=std::fopen(companion(executable,"data/world/endless-presentation.bin").c_str(),"rb");
    if (!file) return false;
    unsigned header[3]{};
    bool ok=std::fread(header,sizeof header,1,file)==1 && header[0]==0x32454355 && header[1]==1 && header[2]==3;
    for (unsigned n=0;ok && n<=wanted;++n) {
        unsigned record[3]{};
        ok=std::fread(record,sizeof record,1,file)==1 && record[0]==512 &&
           record[1]==(n?1024u:512u) && record[2]==record[0]*record[1]/2;
        if (!ok) break;
        if (n!=wanted) ok=std::fseek(file,sizeof palette+record[2],SEEK_CUR)==0;
        else ok=std::fread(palette,sizeof palette,1,file)==1 && std::fread(pixels,record[2],1,file)==1;
    }
    const bool closed=std::fclose(file)==0;
    if (!ok || !closed) {loaded=-1;return false;}
    loaded=static_cast<int>(wanted);
    sceKernelDcacheWritebackRange(palette,sizeof palette);
    sceKernelDcacheWritebackRange(pixels,PixelBytes);
    return true;
}

inline void bind(unsigned tile=0) {
    sceGuClutMode(GU_PSM_8888,0,15,0);sceGuClutLoad(2,palette);
    setTexture(GU_PSM_T4,512,512,pixels+tile*512*512/2);
}
inline void diagram(unsigned index,float x,float y) {
    if (loaded!=0 || index>=6) return;
    bind();patch({static_cast<int16_t>(index%3*160),static_cast<int16_t>(index/3*160),160,160,0,0},x,y,GameplayView::RasterScale);
}
inline void demon(const GameplayView& view,Vec2 at,bool radical,float scale,uint32_t color) {
    if (loaded!=(radical?2:1)) return;
    const Vec2 top=view.position(at+Vec2{-16.384f,radical?-28.672f:-16.384f}*scale);
    const float zoom=view.worldScale()*.064f*scale;
    bind();patch({0,0,512,512,0,0},top.x,top.y,zoom,color);
    if (radical) {bind(1);patch({0,0,512,128,0,0},top.x,top.y+512*zoom,zoom,color);}
}
inline bool prepareCapture() {
    return capture!=nullptr;
}
inline void releaseCapture() {
    // Workspace remains leased until leaving the Endless title. No allocation.
}

inline void effects(const EndlessPresentation::State& state,const GameplayView& view,bool radical) {
    if ((!state.camera() && !state.scareUs) || !capture) return;
    constexpr int Field=272,Left=ViewX+GameplayView::HudWidth;
    auto* source=static_cast<unsigned char*>(sceGeEdramGetAddr())+reinterpret_cast<uintptr_t>(screenDrawBuffer);
    EndlessUi067::invalidate();
    sceGuCopyImage(GU_PSM_5650,Left,0,Field,Field,Stride,source,0,0,Stride,capture);
    // Sony libgu Reference pp233-234: both are required before sampling a copy.
    sceGuTexFlush();sceGuTexSync();
    setTexture(GU_PSM_5650,512,512,capture);
    sceGuTexMode(GU_PSM_5650,0,0,GU_FALSE);
    sceGuTexFilter(GU_NEAREST,GU_NEAREST);
    sceGuTexWrap(GU_CLAMP,GU_CLAMP);
    sceGuScissor(Left,0,Left+Field,Height);
    const auto focus=view.position(state.camera()?state.center:state.originalCenter);
    const float ratio=state.cameraRatio(),time=state.clockUs/1000000.f;
    const float opacity=state.scareUs/700000.f;
    constexpr unsigned Side=35,VertexCount=Side*Side,IndexCount=34*34*6;
    constexpr unsigned MeshOffset=512*288*2;
    static_assert(MeshOffset+VertexCount*sizeof(TextureVertex)+IndexCount*sizeof(uint16_t)<=CaptureBytes);
    auto* vertices=reinterpret_cast<TextureVertex*>(reinterpret_cast<unsigned char*>(capture)+MeshOffset);
    auto* indices=reinterpret_cast<uint16_t*>(vertices+VertexCount);
    for (unsigned y=0;y<Side;++y) for (unsigned x=0;x<Side;++x) {
            const float px=x*8,py=y*8,screenX=(GameplayView::HudWidth+px)/408.f,screenY=py/272.f;
            float u=(px-136)/ratio+(state.camera()?focus.x-Left:136);
            float v=(py-136)/ratio+(state.camera()?focus.y:136);
            unsigned red=255,green=255,blue=255;
            if (state.scareUs) {
                u+=std::cos(time*6+screenY*5.5f)*4.08f*opacity;
                v+=std::cos(time*12+screenX*60)*2.176f*opacity;
                const float wave=.3f*(std::sin(time*2+screenX*9+screenY*2)+std::sin(time*2+screenY+screenX+screenY*5));
                if (radical) {red=static_cast<unsigned>(255*(1-.5f*opacity));blue=static_cast<unsigned>(255*std::clamp(1+(-.5f+wave)*opacity,0.f,1.f));}
                else {red=static_cast<unsigned>(255*std::clamp(1+wave*opacity,0.f,1.f));green=blue=static_cast<unsigned>(255*(1-.5f*opacity));}
            }
            vertices[y*Side+x]={std::clamp(u,0.f,271.99f),std::clamp(v,0.f,271.99f),GU_RGBA(red,green,blue,255),Left+px,py,0};
    }
    unsigned cursor=0;
    for (unsigned y=0;y<34;++y) for (unsigned x=0;x<34;++x) {
        const auto tl=static_cast<uint16_t>(y*Side+x),tr=static_cast<uint16_t>(tl+1);
        const auto bl=static_cast<uint16_t>(tl+Side),br=static_cast<uint16_t>(bl+1);
        indices[cursor++]=tl;indices[cursor++]=tr;indices[cursor++]=br;
        indices[cursor++]=tl;indices[cursor++]=br;indices[cursor++]=bl;
    }
    sceKernelDcacheWritebackRange(vertices,VertexCount*sizeof(TextureVertex)+IndexCount*sizeof(uint16_t));
    constexpr unsigned Format=GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D|GU_INDEX_16BIT;
    sceGuDrawArray(GU_TRIANGLES,Format,IndexCount,indices,vertices);
    sceGuScissor(0,0,Width,Height);
}
}
