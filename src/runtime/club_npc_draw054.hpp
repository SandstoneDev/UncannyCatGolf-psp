#pragma once
// Included inside Club054 after the cached-room renderer.
namespace NpcDraw {
constexpr unsigned PoolStart=1024*1024,PoolEnd=2*1024*1024;
static_assert(RobertOffset+256*128*4<=PoolStart);
struct Instance {unsigned ref,slot,address;float x,y,scale;uint32_t tint;bool above;};
struct Cache {unsigned tile=UINT_MAX,offset=0,bytes=0;};
constexpr unsigned NpcCount=static_cast<unsigned>(ClubTracks::Actor::Count);
std::array<Instance,NpcCount+3> list{};
std::array<Cache,NpcCount+3> cache{};
unsigned count=0,used=0;
std::string path;
void reset() {for(auto& c:cache)c={};}
auto open() {return openAssetFile(path.c_str());}
bool read(std::FILE* f,unsigned offset,unsigned bytes) {
    if(!f || !bytes || bytes>sizeof fallbackPage || std::fseek(f,static_cast<long>(offset),SEEK_SET)!=0)return false;
    fallbackPageIndex=ScreenPages;horseMeter039.invalidateShared();
    return std::fread(fallbackPage,bytes,1,f)==1;
}
bool clean(int roomX,int roomY) {
    auto file=decltype(open()){};
    for(const auto& plate:ClubTracks::plates) {
        if(plate.x!=roomX || plate.y!=roomY)continue;
        if(!file)file=open();
        if(!file || !read(file.get(),plate.offset,plate.bytes))return false;
        const auto* data=reinterpret_cast<const uint8_t*>(fallbackPage);
        if(crc32(0,data,plate.bytes)!=plate.crc)return false;
        auto* pixels=reinterpret_cast<uint16_t*>(reinterpret_cast<uintptr_t>(vram045(RoomOffset))|0x40000000u);
        for(unsigned pos=0;pos<plate.bytes;) {
            if(plate.bytes-pos<6)return false;
            uint16_t span[3];std::memcpy(span,data+pos,6);pos+=6;
            const unsigned x=span[0],y=span[1],n=span[2];
            if(y>=272 || x>=272 || !n || n>272-x || 2*n>plate.bytes-pos)return false;
            for(unsigned i=0;i<n;++i) {
                const unsigned px=x+i;
                const unsigned dest=(((y/8)*64+px/8)*8+y%8)*8+px%8;
                std::memcpy(pixels+dest,data+pos+i*2,2);
            }
            pos+=2*n;
        }
    }
    return true;
}
bool add(unsigned ref,unsigned slot,float x,float y,float scale,uint32_t tint=White,bool above=false) {
    const auto& sprite=ClubTracks::sprites[ref];
    x+=sprite.x*scale;y+=sprite.y*scale;
    if(x+sprite.width*scale<=0 || y+sprite.height*scale<=0 || x>=272 || y>=272)return true;
    const auto& tile=ClubTracks::tiles[sprite.tile];
    const unsigned bytes=unsigned(tile.width)*tile.height*4;
    if(count>=list.size() || bytes>PoolEnd-PoolStart-used)return false;
    list[count++]={ref,slot,PoolStart+used,x,y,scale,tint,above};used+=bytes;
    return true;
}
bool prepare() {
    count=used=0;
    for(unsigned i=0;i<NpcCount;++i) {
        const auto id=static_cast<ClubTracks::Actor>(i);
        const auto& p=visit.people[i];
        if(ClubTracks::baked[i] || !p.visible)continue;
        unsigned ref=ClubTracks::frame(ClubTracks::track(id,p.pose,visit.sockbug),p.ms);
        const auto& original=Geometry::npcs[i].anchor;
        const bool ghost=id==ClubTracks::Actor::AppleJohn && visit.buried;
        const bool scare=id==ClubTracks::Actor::Bobmunculua && visit.bobScare;
        if(ref==ClubTracks::sourceIdle[i] && p.x==original.x && p.y==original.y && !ghost && !scare) {
            if(roomX!=roomIndex(p.x) || roomY!=roomIndex(p.y))continue;
            if(!add(ClubTracks::homeFrames[i],i,0,0,1,White,
                    id==ClubTracks::Actor::AppleJohn || id==ClubTracks::Actor::Bobmunculua))return false;
            continue;
        }
        const auto& t=ClubTracks::transform[i];
        const float zoom=scare?1+39*std::min(1.f,visit.bobMs/1400.f):1;
        const unsigned color=scare?unsigned(255*(1-visit.bobMs/2000.f)):255;
        const uint32_t tint=ghost?0x80ffffffu:0xff0000ffu|(color<<8)|(color<<16);
        if(!add(ref,i,(p.x+t.x*zoom-roomX*Side)*Villa054::Scale,
                (p.y+t.y*zoom-roomY*Side)*Villa054::Scale,t.scale*zoom*Villa054::Scale,tint,
                id==ClubTracks::Actor::AppleJohn || id==ClubTracks::Actor::Bobmunculua))return false;
    }
    if(roomX==-2 && roomY==-1) {
        if(visit.computer && !add(ClubTracks::computerFrames[visit.computer-1],NpcCount,0,0,1))return false;
        if(visit.greenhead==ClubEvents::Greenhead::Alive) {
            if(!add(ClubTracks::greenheadHome,NpcCount+1,0,0,1))return false;
        } else if(visit.greenhead==ClubEvents::Greenhead::Explosion) {
            const auto& t=ClubTracks::tracks[ClubTracks::indices[25][unsigned(ClubTracks::Pose::Explosion)]];
            if(!add(ClubTracks::frame(t,visit.boomMs),NpcCount+1,256*Villa054::Scale,192*Villa054::Scale,2*Villa054::Scale))return false;
        }
    }
    auto file=decltype(open()){};
    for(unsigned i=0;i<count;++i) {
        const auto& n=list[i];const auto tileId=ClubTracks::sprites[n.ref].tile;
        auto& cached=cache[n.slot];const auto& tile=ClubTracks::tiles[tileId];
        const unsigned bytes=unsigned(tile.width)*tile.height*4;
        if(cached.tile==tileId && cached.offset==n.address)continue;
        for(auto& c:cache)
            if(c.tile!=UINT_MAX && c.offset<n.address+bytes && n.address<c.offset+c.bytes)c={};
        if(!file)file=open();
        if(!file || !read(file.get(),tile.offset,tile.bytes))return false;
        auto* output=reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(vram045(n.address))|0x40000000u);
        unsigned long decoded=bytes,packed=tile.bytes;
        if(uncompress2(output,&decoded,reinterpret_cast<const uint8_t*>(fallbackPage),&packed)!=Z_OK ||
           decoded!=bytes || packed!=tile.bytes)return false;
        cached={tileId,n.address,bytes};
    }
    return true;
}
void draw(bool above) {
    sceGuScissor(ViewX+136,0,ViewX+408,Height);
    for(unsigned i=0;i<count;++i) {
        const auto& n=list[i];if(n.above!=above)continue;
        const auto& p=ClubTracks::sprites[n.ref];const auto& tile=ClubTracks::tiles[p.tile];
        setTexture(GU_PSM_8888,tile.width,tile.height,vram045(n.address));
        patch({0,0,static_cast<int16_t>(p.width),static_cast<int16_t>(p.height),0,0},
              ViewX+136+n.x,n.y,n.scale,n.tint);
    }
    sceGuScissor(0,0,Width,Height);
}
void drawScareFloor() {
    if(!visit.bobScare || roomX!=1 || roomY!=3)return;
    const uint32_t c=unsigned(255*(1-std::min(1.f,visit.bobMs/1400.f)));
    for(auto rect:ClubTracks::bobFloor) {
        patch({static_cast<int16_t>(rect.x),static_cast<int16_t>(rect.y),
               static_cast<int16_t>(rect.w),static_cast<int16_t>(rect.h),0,0},
              ViewX+136+rect.x,rect.y,1,0xff0000ffu|(c<<8)|(c<<16));
    }
}
} // namespace NpcDraw
