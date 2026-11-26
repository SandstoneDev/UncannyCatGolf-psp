#pragma once

#include "game.hpp"

namespace EndlessE04Art {

enum : unsigned {
    Key, Hint, Crusher, WideCrusher, AntiCheat,
    PortalBlue, PortalBlueSink, PortalRed, PortalRedSink, Breakable, Speed, Jump
};

constexpr unsigned AtlasWidth=256,AtlasHeight=64;

struct Sprite {
    float u,v,textureWidth,textureHeight;
    float worldWidth,worldHeight;
};

constexpr Sprite sprites[] = {
    {48,0,32,32,32,32}, {80,0,32,32,32,32},
    {112,32,64,17,64,17}, {112,32,64,17,96,17},
    {176,0,17,64,17*12.7763f,64*30.4305f},
    {112,0,32,32,32,32}, {48,32,32,32,32,32},
    {144,0,32,32,32,32}, {80,32,32,32,32,32},
    {0,0,48,64,48,64}, {224,0,32,32,32,32}, {224,32,32,32,32,32}
};
constexpr unsigned AtlasBytes=AtlasWidth*AtlasHeight*sizeof(uint32_t);
static_assert(AtlasBytes<=sizeof fallbackPage);
bool ready=false;

bool load(const char* executable) {
    ready=false;
    fallbackPageIndex=ScreenPages;
    auto* file=std::fopen(companion(executable,"data/world/endless-04-art.bin").c_str(),"rb");
    if (!file) return false;
    char magic[4]{};
    const bool ok=std::fread(magic,sizeof magic,1,file)==1 && std::memcmp(magic,"UCE4",4)==0 &&
        std::fread(fallbackPage,AtlasBytes,1,file)==1 &&
        std::fgetc(file)==EOF && !std::ferror(file);
    std::fclose(file);
    if (!ok) return false;
    sceKernelDcacheWritebackRange(fallbackPage,AtlasBytes);
    ready=true;
    return true;
}

bool draw(unsigned id,Vec2 center,float worldToScreen,unsigned tick) {
    if (!ready || id>=std::size(sprites) || worldToScreen<=0) return false;
    constexpr float FieldPixels=272,fieldX=ViewX+136;
    const auto& sprite=sprites[id];
    const Vec2 half{sprite.worldWidth*worldToScreen*.5f,sprite.worldHeight*worldToScreen*.5f};
    const Vec2 topLeft=center-half,bottomRight=center+half;
    constexpr unsigned Format=GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D;
    setTexture(GU_PSM_8888,AtlasWidth,AtlasHeight,fallbackPage);
    if (id==PortalBlue || id==PortalRed) {
        const float angle=.1f*tick,c=std::cos(angle),s=std::sin(angle);
        auto* vertices=static_cast<TextureVertex*>(sceGuGetMemory(4*sizeof(TextureVertex)));
        for (unsigned k=0;k<4;++k) {
            const unsigned right=k&1,bottom=k>>1;
            const float dx=right?half.x:-half.x,dy=bottom?half.y:-half.y;
            vertices[k]={sprite.u+sprite.textureWidth*right,sprite.v+sprite.textureHeight*bottom,White,
                         center.x+c*dx-s*dy,center.y+s*dx+c*dy,0};
        }
        sceGuDrawArray(GU_TRIANGLE_STRIP,Format,4,nullptr,vertices);
        return true;
    }

    const float x0=std::max(topLeft.x,fieldX),y0=std::max(topLeft.y,0.0f);
    const float x1=std::min(bottomRight.x,fieldX+FieldPixels),y1=std::min(bottomRight.y,FieldPixels);
    if (x1<=x0 || y1<=y0) return true;
    const auto u=[&](float x) {
        return sprite.u+std::clamp((x-topLeft.x)/(2*half.x),0.0f,1.0f)*sprite.textureWidth;
    };
    const auto v=[&](float y) {
        return sprite.v+std::clamp((y-topLeft.y)/(2*half.y),0.0f,1.0f)*sprite.textureHeight;
    };
    unsigned alpha=(id==Hint)?160:255;
    if (id==Breakable) {
        const float phase=tick/30.0f+10*(center.x-fieldX)/FieldPixels+4*center.y/FieldPixels;
        alpha=static_cast<unsigned>(204+51*std::cos(phase)+.5f);
    }
    const auto tint=GU_RGBA(255,255,255,alpha);
    auto* vertices=static_cast<TextureVertex*>(sceGuGetMemory(2*sizeof(TextureVertex)));
    vertices[0]={u(x0),v(y0),tint,x0,y0,0};
    vertices[1]={u(x1),v(y1),tint,x1,y1,0};
    sceGuDrawArray(GU_SPRITES,Format,2,nullptr,vertices);
    return true;
}

} // namespace EndlessE04Art
