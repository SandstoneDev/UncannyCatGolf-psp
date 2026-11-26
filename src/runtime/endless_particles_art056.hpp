#pragma once
#include "endless_particles056.hpp"

namespace EndlessParticlesArt056 {
using namespace EndlessParticles056;
constexpr unsigned TextureBytes=TextureSide*TextureSide;
alignas(64) inline uint8_t textures[2][TextureBytes];
alignas(64) inline uint32_t palettes[2][256],haloPalettes[2][256];
alignas(64) inline int8_t noisePixels[NoiseBytes];
alignas(64) inline TextureVertex particleVertices[Amount*2*2];
alignas(64) inline TextureVertex haloVertices[2];
inline bool loaded=false;
static_assert(sizeof(TextureVertex)==24);
static_assert(sizeof(textures)+sizeof(palettes)+sizeof(haloPalettes)+sizeof(noisePixels)+sizeof(particleVertices)+sizeof(haloVertices)==89136);
inline unsigned gain(unsigned channel,unsigned alpha,float color){return std::min(255u,static_cast<unsigned>((channel*alpha*2*color+127)/255));}
inline bool load(const char* executable){
    if(loaded)return true;
    const auto input=openAssetFile(assetPath(executable,"endless-particles056.bin").c_str());
    auto* file=input.get();
    if(!file)return false;
    uint32_t header[5]{};
    bool ok=std::fread(header,sizeof header,1,file)==1 && header[0]==0x36504355 && header[1]==1 && header[2]==128 && header[3]==32 && header[4]==8;
    for(unsigned i=0;ok && i<2;++i)ok=std::fread(palettes[i],sizeof palettes[i],1,file)==1 && std::fread(textures[i],sizeof textures[i],1,file)==1;
    ok=ok && std::fread(noisePixels,sizeof noisePixels,1,file)==1 && std::fgetc(file)==EOF && !std::ferror(file);
    if(!ok)return false;
    for(unsigned i=0;i<256;++i){
        const uint32_t p=palettes[1][i];const unsigned r=p&255,g=(p>>8)&255,b=(p>>16)&255,a=p>>24;
        // PointLight energy=2: premultiply the original light texture and the
        // exact scene tint into a saturated additive GE CLUT. No extra pass.
        haloPalettes[0][i]=GU_RGBA(gain(r,a,1),0,0,255);
        haloPalettes[1][i]=GU_RGBA(gain(r,a,.359592f),gain(g,a,.479432f),gain(b,a,1),255);
    }
    sceKernelDcacheWritebackRange(textures,sizeof textures);
    sceKernelDcacheWritebackRange(palettes,sizeof palettes);
    sceKernelDcacheWritebackRange(haloPalettes,sizeof haloPalettes);
    noiseField=noisePixels;loaded=true;return true;
}
inline void bind(unsigned texture,const uint32_t* palette){
    sceGuClutMode(GU_PSM_8888,0,255,0);sceGuClutLoad(32,palette);
    setTexture(GU_PSM_T8,128,128,textures[texture]);
}
inline bool quad(TextureVertex* vertices,Vec2 center,float size,uint32_t color){
    constexpr float Left=ViewX+GameplayView::HudWidth,Right=Left+272;
    const float x=center.x-size*.5f,y=center.y-size*.5f;
    const float l=std::max(Left,x),t=std::max(0.f,y),r=std::min(Right,x+size),b=std::min(272.f,y+size);
    if(l>=r || t>=b)return false;
    const float uv=128/size;
    vertices[0]={(l-x)*uv,(t-y)*uv,color,l,t,0};
    vertices[1]={std::min(128.f,(r-x)*uv),std::min(128.f,(b-y)*uv),color,r,b,0};return true;
}
inline void draw(const State& state,const GameplayView& view,Vec2 demon){
    if(!loaded)return;
    unsigned count=0;
    for(unsigned i=0;i<state.particles.size();++i){
        const auto& p=state.particles[i];
        const unsigned alpha=static_cast<unsigned>(255*state.renderOpacity(p)*(i<Amount?BlackAlpha:ColorAlpha));
        if(!alpha)continue;
        const uint32_t tint=i<Amount?GU_RGBA(0,0,0,alpha):state.radical?GU_RGBA(114,162,255,alpha):GU_RGBA(255,0,0,alpha);
        if(quad(particleVertices+count,view.position(state.renderPosition(p)),TextureSide*p.scale*view.worldScale(),tint))count+=2;
    }
    constexpr unsigned Format=GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D;
    // These persistent buffers are written once per draw, after main.endFrame's
    // GU_SYNC_FINISH for the preceding frame; never borrowed from card1/capture.
    if(count){
        sceKernelDcacheWritebackRange(particleVertices,count*sizeof(TextureVertex));bind(0,palettes[0]);
        sceGuDrawArray(GU_SPRITES,Format,count,nullptr,particleVertices);
    }
    if(quad(haloVertices,view.position(demon),TextureSide*2*view.worldScale(),0xffffffffu)){
        sceKernelDcacheWritebackRange(haloVertices,sizeof haloVertices);bind(1,haloPalettes[state.radical?1:0]);
        sceGuBlendFunc(GU_ADD,GU_FIX,GU_FIX,0x00ffffff,0x00ffffff);
        sceGuDrawArray(GU_SPRITES,Format,2,nullptr,haloVertices);
        sceGuBlendFunc(GU_ADD,GU_SRC_ALPHA,GU_ONE_MINUS_SRC_ALPHA,0,0);
    }
}
}
