#pragma once

#include "endless_modifiers.hpp"

namespace EndlessModifierArt {

using EndlessModifiers::State;
constexpr unsigned AtlasSide=256;
alignas(64) inline unsigned char pixels[AtlasSide*AtlasSide];
alignas(64) inline uint32_t colors[256];
inline bool loaded=false;

inline bool load(const char* executable) {
    if (loaded) return true;
    auto* file=std::fopen(companion(executable,"data/world/endless-modifiers.bin").c_str(),"rb");
    if (!file) return false;
    char magic[4]{};
    loaded=std::fread(magic,sizeof magic,1,file)==1 && std::memcmp(magic,"UCMF",4)==0 &&
           std::fread(colors,sizeof colors,1,file)==1 && std::fread(pixels,sizeof pixels,1,file)==1 &&
           std::fgetc(file)==EOF && !std::ferror(file);
    std::fclose(file);
    if (loaded) {
        sceKernelDcacheWritebackRange(colors,sizeof colors);
        sceKernelDcacheWritebackRange(pixels,sizeof pixels);
    }
    return loaded;
}

inline void bind() {
    sceGuClutMode(GU_PSM_8888,0,255,0);
    sceGuClutLoad(32,colors);
    setTexture(GU_PSM_T8,AtlasSide,AtlasSide,pixels);
}

inline void sprite(Patch uv,Vec2 center,float width,float height,uint32_t tint=White,bool flip=false) {
    patchScaled(uv,center.x-width*.5f,center.y-height*.5f,width/uv.width,height/uv.height,tint,flip);
}

inline Patch explosion(unsigned frame) {
    frame=std::min(frame,16u);
    return {static_cast<int16_t>(frame%5*36),static_cast<int16_t>(frame/5*47),32,45,0,0};
}


inline void drawGround(const State& state) {
    if (!loaded || !state.enabled(EndlessRules::Pharaoh)) return;
    bind();
    // The original terrain is a floor behind the walls. The PSP field is
    // already flattened, so keep the authored maze readable through the sand.
    patchScaled({192,128,64,64,0,0},ViewX+GameplayView::HudWidth,0,272.f/64,272.f/64,
                GU_RGBA(255,220,123,112));
}

inline uint32_t reticleColor(unsigned tick,unsigned alpha) {
    constexpr Vec2 firstChannel[]={{255,69},{69,255},{162,69},{255,69}};
    constexpr float blue[]={69,193,255,69};
    const unsigned t=tick%120;
    const unsigned section=t<60?0:t<90?1:2;
    const float part=section==0?t/60.0f:(t-30*section-30)/30.0f;
    const Vec2 rg=firstChannel[section]+(firstChannel[section+1]-firstChannel[section])*part;
    const float b=blue[section]+(blue[section+1]-blue[section])*part;
    return GU_RGBA(static_cast<unsigned>(rg.x),static_cast<unsigned>(rg.y),static_cast<unsigned>(b),alpha);
}

inline void drawMissile(const EndlessModifiers::Missile& missile,unsigned ticks,const GameplayView& view) {
    const float scale=view.worldScale();
    const auto center=view.position(missile.position);
    const unsigned frame=missile.animation/6%4;
    sprite({192,224,30,30,0,0},center,256*scale,256*scale,reticleColor(ticks,48));
    sprite({static_cast<int16_t>(180+frame%2*36),static_cast<int16_t>(frame/2*35),32,32,0,0},
           center,128*scale,128*scale,reticleColor(ticks,255));
    if (missile.blast && missile.blast<=41)
        sprite(explosion((missile.blast-1)*25/60),view.position(missile.position+Vec2{0,16}),
               208*scale,224*scale);
}

inline void drawMissile(const State& state,const GameplayView& view) { drawMissile(state.missile,state.ticks,view); }

inline uint32_t scramble(uint32_t value) {
    value^=value>>15;
    value*=0x2c1b3c6d;
    value^=value>>12;
    value*=0x297a2d39;
    return value^(value>>15);
}

inline void drawSandstorm(const State& state,const GameplayView& view) {
    constexpr unsigned Count=96,Lifetime=120;
    constexpr unsigned Format=GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D;
    auto* vertices=static_cast<TextureVertex*>(sceGuGetMemory(Count*2*sizeof(TextureVertex)));
    unsigned count=0;
    for (unsigned i=0;i<Count;++i) {
        const unsigned offset=i*Lifetime/Count;
        const unsigned age=(state.ticks+offset)%Lifetime;
        const float seconds=age/60.f;
        const uint32_t seed=scramble(i+(state.ticks+offset)/Lifetime*Count+1);
        const float x=static_cast<float>(seed&1023)-512;
        const float y=static_cast<float>((seed>>10)&1023)-512;
        const float vx=static_cast<float>((seed>>20)&127)-63;
        const float vy=static_cast<float>((seed>>25)&127)-63;
        const Vec2 point=view.position({x+vx*seconds-150*seconds*seconds,y+vy*seconds});
        const float weight=age<49?age/49.f:(Lifetime-age)/71.f;
        const float size=(4+((seed>>29)&7))*weight*view.worldScale();
        if (point.x+size<ViewX+GameplayView::HudWidth || point.x-size>ViewX+GameplayView::Width ||
            point.y+size<0 || point.y-size>GameplayView::Height || size<.25f) continue;
        const auto tint=GU_RGBA(229,161+(seed&31),66,static_cast<unsigned>(182*weight));
        vertices[count++]={192,192,tint,point.x-size,point.y-size,0};
        vertices[count++]={208,208,tint,point.x+size,point.y+size,0};
    }
    if (count) sceGuDrawArray(GU_SPRITES,Format,count,nullptr,vertices);
}

inline void drawWorld(const State& state,const GameplayView& view,Vec2 player,const BunkerJumpy& jump) {
    if (loaded && (state.mods&(EndlessRules::Missiles|EndlessRules::Pharaoh))) {
        bind();
        if (state.enabled(EndlessRules::Missiles)) drawMissile(state,view);
        if (state.enabled(EndlessRules::Pharaoh)) drawSandstorm(state,view);
    }
    if (jump.enabled && jump.charge>0) {
        const Vec2 at=view.position(player+Vec2{0,-24});
        const float width=17*std::clamp(1-jump.charge/BunkerJumpy::LaunchDistance,0.f,1.f);
        if (width>0) rectangle(at.x-width*.5f,at.y-1,width,2,GU_RGBA(255,133,55,255));
    }
}

inline Vec2 screen(Vec2 point) {
    return {ViewX+point.x*GameplayView::RasterScale,point.y*GameplayView::RasterScale};
}

inline void drawHud(const State& state) {
    if (!loaded || !(state.mods&(EndlessRules::Bort|EndlessRules::Bees))) return;
    constexpr float Scale=GameplayView::RasterScale;
    sceGuScissor(ViewX,0,ViewX+GameplayView::Width,GameplayView::Height);
    bind();
    if (state.enabled(EndlessRules::Bort)) {
        for (const auto& bird:state.birds) {
            const Vec2 at=screen(bird.position);
            if (bird.live)
                sprite({static_cast<int16_t>(bird.hue*24),static_cast<int16_t>(192+bird.frame*32),22,32,0,0},
                       at,40*Scale,60*Scale);
            else if (bird.blast) sprite(explosion((bird.blast-1)/2),at,71*Scale,100*Scale);
        }
    }
    if (state.enabled(EndlessRules::Bees)) {
        for (const auto& bee:state.bees) {
            const Vec2 at=screen(bee.position);
            if (at.x+16<ViewX || at.x-16>ViewX+GameplayView::Width ||
                at.y+13<0 || at.y-13>GameplayView::Height) continue;
            const unsigned frame=static_cast<unsigned>(state.ticks*bee.animationSpeed/3.0f)%4;
            sprite({static_cast<int16_t>(180+frame%2*36),static_cast<int16_t>(72+frame/2*28),32,26,0,0},
                   at,150*.401f*Scale,120*.401f*Scale,White,bee.velocity.x<0);
        }
    }
    sprite({static_cast<int16_t>(192+(state.ticks/4%4)*13),209,13,13,0,0},screen(state.cursor),
           13*Scale,13*Scale);
    sceGuScissor(0,0,Width,Height);
}

} // namespace EndlessModifierArt
