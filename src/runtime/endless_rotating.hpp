#pragma once

#include "game.hpp"
#include <new>

namespace EndlessRotating {

constexpr unsigned PeriodTicks=8*60;
constexpr unsigned AtlasWidth=128,AtlasHeight=64;
constexpr unsigned FieldPixels=272,HudPixels=136;
constexpr unsigned TileCount=16+72+64;

struct Tile {
    int8_t x,y;
    uint8_t u,v;
};
static_assert(sizeof(Tile)==4);

struct Rotation {
    float c,s;
    Vec2 toLocal(Vec2 p) const { return {c*p.x+s*p.y,c*p.y-s*p.x}; }
    Vec2 toWorld(Vec2 p) const { return {c*p.x-s*p.y,s*p.x+c*p.y}; }
};

struct Ring {
    unsigned tileOffset,tileCount;
    float start,end;
    TileField walls{};
    Rotation previous{1,0},current{1,0};
    float previousAngle=0,currentAngle=0;
};

// TileRotate animation in the original E_05.tscn, with an eight second linear loop.
struct Storage {
    Ring rings[3]={{0,16,0,12.5664f},{16,72,0,-6.28319f},{88,64,.785398f,13.3518f}};
    std::array<Tile,TileCount> tiles;
    alignas(64) std::array<uint32_t,AtlasWidth*AtlasHeight> atlas;
};
static_assert(sizeof(Storage)<=sizeof fallbackPage);
Storage* storage=nullptr;
bool ready=false;
float shimmerPhase=0;

Rotation rotation(float angle) { return {std::cos(angle),std::sin(angle)}; }

void tick(unsigned elapsedTicks) {
    const unsigned current=elapsedTicks%PeriodTicks;
    const unsigned previous=elapsedTicks?(elapsedTicks-1)%PeriodTicks:0;
    const unsigned endSample=(elapsedTicks && !current)?PeriodTicks:current;
    for (auto& ring:storage->rings) {
        const float span=ring.end-ring.start;
        ring.previousAngle=ring.start+span*previous/PeriodTicks;
        ring.currentAngle=ring.start+span*endSample/PeriodTicks;
        ring.previous=rotation(ring.previousAngle);
        ring.current=rotation(ring.start+span*current/PeriodTicks);
    }
    shimmerPhase=2.0f*(elapsedTicks/60.0f);
}

bool load(const char* executable) {
    ready=false;
    // The background fallback buffer is otherwise idle while playing a native Endless board.
    fallbackPageIndex=ScreenPages;
    storage=new (fallbackPage) Storage{};
    auto& [rings,tiles,atlas]=*storage;
    for (unsigned i=0;i<std::size(rings);++i) {
        char name[48];
        std::snprintf(name,sizeof name,"data/world/endless-05-rotate-%u.bin",i+2);
        auto* file=std::fopen(companion(executable,name).c_str(),"rb");
        if (!file) return false;
        const bool ok=rings[i].walls.read(file);
        std::fclose(file);
        if (!ok) return false;
    }

    auto* file=std::fopen(companion(executable,"data/world/endless-05-rings.bin").c_str(),"rb");
    if (!file) return false;
    char magic[4]{};
    uint16_t counts[4]{};
    const bool ok=std::fread(magic,sizeof magic,1,file)==1 && std::memcmp(magic,"UCR5",4)==0 &&
        std::fread(counts,sizeof counts,1,file)==1 &&
        counts[0]==rings[0].tileCount && counts[1]==rings[1].tileCount && counts[2]==rings[2].tileCount && !counts[3] &&
        std::fread(tiles.data(),sizeof(Tile),tiles.size(),file)==tiles.size() &&
        std::fread(atlas.data(),sizeof(uint32_t),atlas.size(),file)==atlas.size() &&
        std::fgetc(file)==EOF && !std::ferror(file);
    std::fclose(file);
    if (!ok || !std::all_of(tiles.begin(),tiles.end(),[](Tile tile) {
        return tile.x>=-24 && tile.x<24 && tile.y>=-24 && tile.y<24 &&
            tile.u<AtlasWidth/16 && tile.v<AtlasHeight/16;
    })) return false;

    sceKernelDcacheWritebackRange(atlas.data(),atlas.size()*sizeof(uint32_t));
    tick(0);
    ready=true;
    return true;
}

WallContact sweep(Vec2 position,Vec2 motion,float radius) {
    WallContact result;
    if (!ready) return result;
    for (const auto& ring:storage->rings) {
        // Transform both ends into the rotating reference frame so advancing walls
        // also participate in the sweep between two consecutive authored poses.
        const auto start=ring.previous.toLocal(position);
        const auto end=ring.current.toLocal(position+motion);
        const auto hit=ring.walls.sweep(start,end-start,radius);
        if (hit.time>=result.time) continue;
        const float elapsed=std::clamp(hit.time,0.0f,1.0f);
        const float angle=ring.previousAngle+(ring.currentAngle-ring.previousAngle)*elapsed;
        result={hit.time,rotation(angle).toWorld(hit.normal)};
    }
    return result;
}

Vec2 recover(Vec2 position,float radius) {
    if (!ready) return {};
    Vec2 displace{};
    for (const auto& ring:storage->rings) {
        const auto local=ring.current.toLocal(position+displace);
        const auto correction=ring.walls.recoverCircle(local,radius);
        displace=displace+ring.current.toWorld(correction);
    }
    return displace;
}

void draw(float fieldLeft,float fieldTop,float fieldSize) {
    if (!ready || fieldSize<=0) return;
    const float scale=FieldPixels/fieldSize;
    const int fieldX=ViewX+HudPixels;
    constexpr unsigned Corners[6]={0,1,2,2,1,3};
    sceGuScissor(fieldX,0,fieldX+FieldPixels,FieldPixels);
    const auto& [rings,tiles,atlas]=*storage;
    setTexture(GU_PSM_8888,AtlasWidth,AtlasHeight,atlas.data());
    for (const auto& ring:rings) {
        auto* vertices=static_cast<TextureVertex*>(sceGuGetMemory(ring.tileCount*6*sizeof(TextureVertex)));
        for (unsigned i=0;i<ring.tileCount;++i) {
            const auto& tile=tiles[ring.tileOffset+i];
            TextureVertex quad[4];
            for (unsigned k=0;k<std::size(quad);++k) {
                const unsigned right=k&1,bottom=k>>1;
                const auto p=ring.current.toWorld({16.0f*(tile.x+static_cast<int>(right)),16.0f*(tile.y+static_cast<int>(bottom))});
                const float u=(p.x-fieldLeft)/fieldSize,v=(p.y-fieldTop)/fieldSize;
                const float glow=.8f+.2f*std::cos(shimmerPhase+10*u+4*v);
                const auto opacity=static_cast<unsigned>(255*glow+.5f);
                quad[k]={16.0f*(tile.u+right),16.0f*(tile.v+bottom),GU_RGBA(255,255,255,opacity),
                         fieldX+(p.x-fieldLeft)*scale,(p.y-fieldTop)*scale,0};
            }
            for (unsigned j=0;j<std::size(Corners);++j) vertices[i*6+j]=quad[Corners[j]];
        }
        sceGuDrawArray(GU_TRIANGLES,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,
                       ring.tileCount*6,nullptr,vertices);
    }
    sceGuScissor(0,0,Width,Height);
}

} // namespace EndlessRotating
