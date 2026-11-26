#pragma once

#include "endless_objects.hpp"

namespace EndlessObjectsArt {

constexpr unsigned Width=128,Height=128,Bytes=Width*Height*sizeof(uint16_t);
constexpr unsigned WorkspaceOffset=144*1024;
static_assert(WorkspaceOffset+Bytes<=sizeof fallbackPage);
static_assert(ChuckleArt::TextureBytes<=WorkspaceOffset);
static_assert(sizeof(EndlessRotating::Storage)<=WorkspaceOffset);
static_assert(EndlessE04Art::AtlasBytes<=WorkspaceOffset);
bool ready=false,hasChuckles=false;

uint8_t* pixels() { return reinterpret_cast<uint8_t*>(fallbackPage)+WorkspaceOffset; }

bool readAtlas(const std::string& path,const char (&expected)[5],void* out,unsigned bytes) {
    auto* f=std::fopen(path.c_str(),"rb");
    if (!f) return false;
    char magic[4]{};
    const bool ok=std::fread(magic,sizeof magic,1,f)==1 && !std::memcmp(magic,expected,4) &&
        std::fread(out,bytes,1,f)==1 && std::fgetc(f)==EOF && !std::ferror(f);
    std::fclose(f);
    if (ok) sceKernelDcacheWritebackRange(out,bytes);
    return ok;
}

bool load(const char* executable,unsigned hole) {
    ready=false;
    fallbackPageIndex=ScreenPages;
    hasChuckles=hole==3 || hole==7;
    ready=readAtlas(companion(executable,"data/world/endless-objects-art.bin"),"UCO1",pixels(),Bytes) &&
        // E_03 and E_07 have no course-specific atlas in the lower region.
        (!hasChuckles || readAtlas(companion(executable,"data/world/chuckle.pak"),"UCC1",
                                  fallbackPage,ChuckleArt::TextureBytes));
    return ready;
}

void bind() { setTexture(GU_PSM_4444,Width,Height,pixels()); }

struct Span { float offset; int texture,length; };

unsigned spans(float size,Span (&out)[8]) {
    unsigned count=0;
    out[count++]={0,0,4};
    const int length=static_cast<int>(size);
    for (int start=4;start<length-4;start+=24)
        out[count++]={static_cast<float>(start),4,std::min(24,length-4-start)};
    out[count++]={size-4,28,4};
    return count;
}

void panel(const GameplayView& view,Vec2 center,float width,float height,bool door,uint32_t tint) {
    Span x[8],y[8];
    const unsigned nx=spans(width,x),ny=spans(height,y);
    const float scale=view.worldScale();
    const Vec2 topLeft=view.position(center)-Vec2{width,height}*(scale*.5f);
    const unsigned layers=door?2:1;
    auto* vertex=static_cast<TextureVertex*>(sceGuGetMemory(layers*nx*ny*2*sizeof(TextureVertex)));
    auto* cursor=vertex;
    for (unsigned layer=0;layer<layers;++layer) {
        const float u=door?32.0f*layer:64.0f;
        const uint32_t color=door && layer==0?White:tint;
        for (unsigned row=0;row<ny;++row) {
            for (unsigned col=0;col<nx;++col) {
                const auto& s=x[col];
                const auto& t=y[row];
                *cursor++={u+s.texture,static_cast<float>(t.texture),color,
                           topLeft.x+s.offset*scale,topLeft.y+t.offset*scale,0};
                *cursor++={u+s.texture+s.length,static_cast<float>(t.texture+t.length),color,
                           topLeft.x+(s.offset+s.length)*scale,topLeft.y+(t.offset+t.length)*scale,0};
            }
        }
    }
    sceGuDrawArray(GU_SPRITES,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,
                   cursor-vertex,nullptr,vertex);
}

void drawFlag(const GameplayView& view,Vec2 goal,unsigned tick) {
    if (!ready) return;
    constexpr Patch frames[]={{32,96,24,32,-12,-16},{56,96,24,32,-12,-16},
                              {80,96,24,32,-12,-16},{104,0,24,32,-12,-16}};
    bind();
    actor(view,frames[tick/5%std::size(frames)],goal+Vec2{32,-48},view.worldScale()*4);
}

void drawItems(const GameplayView& view,const EndlessObjects::Level& stage,const EndlessObjects::State& state,
               const std::array<bool,256>& collected,unsigned tick,unsigned switchState,bool e04) {
    using EndlessObjects::Enemy;
    using EndlessObjects::Kind;
    if (!ready) return;
    const float scale=view.worldScale();
    bind();
    if (!e04) {
        constexpr Patch jump[]={{0,64,32,32,-16,-16},{32,64,32,32,-16,-16},
                                {64,64,32,32,-16,-16},{0,96,32,32,-16,-16}};
        for (unsigned i=0;i<stage.count;++i) {
            if (collected[i]) continue;
            const auto& item=stage.items[i];
            const Vec2 center{item.x,item.y};
            switch (item.kind) {
            case Kind::JumpPad: {
                const int quarter=static_cast<int>(std::round(item.a/1.57079633f));
                actor(view,jump[(quarter+4)%4],center,scale);
                break;
            }
            case Kind::SpeedToken: actor(view,{32,32,32,32,-16,-16},center,scale); break;
            case Kind::StopToken: actor(view,{64,32,32,32,-16,-16},center,scale); break;
            case Kind::Coin: actor(view,{0,32,16,16,-8,-8},center,1.5f*scale); break;
            case Kind::Switch: {
                const auto v=static_cast<int16_t>((item.arg?0:switchState)*32+32);
                actor(view,{96,v,32,32,-16,-16},center,scale);
                break;
            }
            case Kind::Door: {
                panel(view,center,item.a*2,item.b*2,true,item.color);
                const unsigned door=state.doorIndex[i];
                const auto alpha=door<state.keyStage.doorCount?static_cast<unsigned>(255*state.keys.lockAlpha[door]):255u;
                if (alpha) actor(view,{16,32,16,16,-8,-8},center,scale,GU_RGBA(255,255,255,alpha));
                break;
            }
            case Kind::Block: {
                const Vec2 at=view.position(center);
                const float phase=tick/30.0f+10*(at.x-ViewX-136)/272+4*at.y/272;
                const auto alpha=static_cast<unsigned>(204+51*std::cos(phase)+.5f);
                panel(view,center,item.a*2,item.b*2,false,GU_RGBA(255,255,255,alpha));
                break;
            }
            default: break;
            }
        }
    }
    for (unsigned i=0;i<state.carCount;++i)
        if (state.cars[i].halfSize.x>0) {
            actor(view,{104,96,24,32,-12,-16},state.cars[i].center,scale);
            ActorLabelDraw054::add(state.carActors[i].label,view,state.cars[i].center,ActorPronouns054::Kind::Baddie);
        }

    if (state.keyStage.keyCount) {
        setTexture(GU_PSM_8888,256,256,roundKeySprites);
        const unsigned frame=tick/3%KeyFrames;
        const Patch sprite{static_cast<int16_t>(frame%10*24),static_cast<int16_t>(frame/10*24),17,17,-8,-8};
        for (unsigned i=0;i<state.keyStage.keyCount;++i) {
            const uint32_t bit=1u<<i;
            const bool holding=state.keys.available&bit;
            if (!holding && (state.keys.collected&bit)) continue;
            const uint32_t color=state.keyStage.keys[i].color;
            const uint32_t tint=holding?(color&0xffffffu)|0x80000000u:color;
            actor(view,sprite,state.keys.motion[i].position,scale*2,tint);
        }
    }
    if (state.spawnerCount) {
        setTexture(GU_PSM_8888,BaddieArtWidth,BaddieArtHeight,baddiePixels);
        for (unsigned i=0;i<state.spawnerCount;++i) {
            const auto& source=state.spawners[i];
            actor(view,{static_cast<int16_t>(source.frame(tick)*24),0,17,17,-8,-8},source.center,scale*2);
        }
    }
    if (state.enemyCount) {
        setTexture(GU_PSM_8888,256,256,interactiveTexture);
        const unsigned frame=tick*24/60%24;
        const Patch skull{static_cast<int16_t>((frame%8)*32),static_cast<int16_t>((frame/8)*32),26,26,-13,-13};
        for (unsigned i=0;i<state.enemyCount;++i)
            if (state.enemies[i].radius>0 && state.enemyActors[i].kind==Enemy::Skull) {
                actor(view,skull,state.enemies[i].center,scale*2);
                ActorLabelDraw054::add(state.enemyActors[i].label,view,state.enemies[i].center,ActorPronouns054::Kind::Baddie);
            }
        if (hasChuckles) {
            setTexture(GU_PSM_8888,ChuckleArt::Width,ChuckleArt::Height,fallbackPage);
            for (unsigned i=0;i<state.enemyCount;++i)
                if (state.enemies[i].radius>0 && state.enemyActors[i].kind==Enemy::Chuckle) {
                    actor(view,ChuckleArt::frame(tick-state.enemyActors[i].bornAt),state.enemies[i].center,scale);
                    ActorLabelDraw054::add(state.enemyActors[i].label,view,state.enemies[i].center,ActorPronouns054::Kind::Baddie);
                }
        }
    }
    for (unsigned i=0;i<state.enemyCount;++i) {
        const auto& enemy=state.enemyActors[i];
        if (enemy.defeatedAt && tick-enemy.defeatedAt<50) ActorLabelDraw054::add(enemy.label,view,state.enemies[i].center,ActorPronouns054::Kind::Baddie);
        if (enemy.defeatedAt) drawDestruction(view,state.enemies[i].center,tick-enemy.defeatedAt,enemy.points);
    }
    for (unsigned i=0;i<state.carCount;++i)
        if (state.carActors[i].defeatedAt) {
            const auto age=tick-state.carActors[i].defeatedAt;
            if (age<50) ActorLabelDraw054::add(state.carActors[i].label,view,state.cars[i].center,ActorPronouns054::Kind::Baddie);
            drawDestruction(view,state.cars[i].center,age,0);
        }
    for (unsigned i=0;i<stage.count;++i) {
        if (stage.items[i].kind!=Kind::Door) continue;
        const unsigned door=state.doorIndex[i];
        const unsigned time=door<state.keyStage.doorCount?state.keys.openedAt[door]:0;
        if (time) drawDestruction(view,state.keyStage.doors[door].center,tick-time,stage.items[i].arg);
    }
}

} // namespace EndlessObjectsArt
