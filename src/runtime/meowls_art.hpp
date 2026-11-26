#pragma once

#include "asset_file.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <pspge.h>
#include <pspgu.h>
#include <pspkernel.h>
#include <zlib.h>

#include "meowls_art_data.hpp"

namespace MeowlsArt {

enum class Graphic : unsigned {
    Player, Angry, Caholic, Officer, OfficerShot, Thhead, ThheadWarning,
    Egg, Gator, Meowling, Bottle, Feather, Dotted, Burst, Blood, Pole, Count,
    HealthHead=Thhead, HealthTail=ThheadWarning
};

inline constexpr float Scale=17.0f/32.0f;
inline constexpr unsigned Backgrounds=13;
inline constexpr unsigned SheetCount=std::size(Packed::Pages);
inline constexpr unsigned WorldBytes=2*272*272;
inline constexpr unsigned Start=0x88000;
inline constexpr unsigned DisplayFx=GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D;
inline constexpr unsigned ViewRight=408;
inline constexpr unsigned StageLeft=136;
static_assert(std::size(Packed::Graphics)==static_cast<unsigned>(Graphic::Count));

inline constexpr std::array<unsigned,SheetCount+1> offsets() {
    std::array<unsigned,SheetCount+1> result{};
    result[0]=Start+WorldBytes;
    for (unsigned i=0;i<SheetCount;++i)
        result[i+1]=result[i]+Packed::Pages[i].width*Packed::Pages[i].height*2;
    return result;
}
inline constexpr auto SheetOffsets=offsets();
static_assert(SheetOffsets.back()<=0x1a0000);

struct Record { uint32_t offset,bytes,raw; };
inline std::array<Record,SheetCount+Backgrounds> records{};
inline AssetFile catalog;
inline unsigned currentWorld=Backgrounds;

inline uint8_t* video(unsigned offset) {
    return static_cast<uint8_t*>(sceGeEdramGetAddr())+offset;
}

inline bool inflateToVideo(FILE* file,const Record& record,unsigned dest) {
    if (std::fseek(file,record.offset,SEEK_SET)!=0) return false;
    z_stream stream{};
    if (inflateInit(&stream)!=Z_OK) return false;
    alignas(64) uint8_t input[4096],output[4096];
    unsigned written=0, unread=record.bytes;
    auto* target=reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(video(dest))|0x40000000u);
    bool good=false;
    for (;;) {
        if (!stream.avail_in && unread) {
            const unsigned count=std::min<unsigned>(sizeof input,unread);
            if (std::fread(input,1,count,file)!=count) break;
            stream.next_in=input;
            stream.avail_in=count;
            unread-=count;
        }
        stream.next_out=output;
        stream.avail_out=sizeof output;
        const int mode=inflate(&stream,Z_NO_FLUSH);
        const unsigned count=sizeof output-stream.avail_out;
        if ((mode!=Z_OK && mode!=Z_STREAM_END) || count>record.raw-written) break;
        std::memcpy(target+written,output,count);
        written+=count;
        if (mode==Z_STREAM_END) {
            good=written==record.raw && !unread && !stream.avail_in;
            break;
        }
    }
    inflateEnd(&stream);
    return good;
}

inline bool world(unsigned zeroBased) {
    const unsigned next=zeroBased%Backgrounds;
    if (!catalog) return false;
    if (currentWorld==next) return true;
    currentWorld=Backgrounds;
    if (!inflateToVideo(catalog.get(),records[SheetCount+next],Start)) return false;
    currentWorld=next;
    return true;
}

inline void unload() {
    catalog.reset();
    currentWorld=Backgrounds;
}

inline bool load(const char* executable) {
    unload();
    if (static_cast<unsigned>(sceGeEdramGetSize())<SheetOffsets.back()) return false;
    auto source=openAssetFile(companion(executable,"data/world/meowls-art.bin").c_str());
    if (!source) return false;
    uint32_t signature=0,count=0;
    constexpr uint32_t Magic=0x574d4355;  // UCMW
    if (std::fread(&signature,sizeof signature,1,source.get())!=1 || signature!=Magic ||
        std::fread(&count,sizeof count,1,source.get())!=1 || count!=records.size() ||
        std::fread(records.data(),sizeof(Record),records.size(),source.get())!=records.size() ||
        std::fseek(source.get(),0,SEEK_END)!=0) return false;
    const auto length=std::ftell(source.get());
    if (length<0) return false;
    const auto size=static_cast<unsigned long>(length);
    unsigned next=8+records.size()*sizeof(Record);
    for (unsigned i=0;i<records.size();++i) {
        const auto& r=records[i];
        const auto expected=i<SheetCount?SheetOffsets[i+1]-SheetOffsets[i]:WorldBytes;
        if (r.raw!=expected || r.offset!=next || next>size || !r.bytes || r.bytes>size-next)
            return false;
        next+=r.bytes;
    }
    if (next!=size) return false;
    for (unsigned i=0;i<SheetCount;++i)
        if (!inflateToVideo(source.get(),records[i],SheetOffsets[i])) return false;
    catalog=std::move(source);
    if (world(0)) return true;
    unload();
    return false;
}

inline void fullView() {
    sceGuScissor(ViewX,0,ViewX+ViewRight,272);
}

inline void fieldView() {
    sceGuScissor(ViewX+StageLeft,0,ViewX+ViewRight,272);
}

inline void bindPage(unsigned index) {
    const auto& p=Packed::Pages[index];
    setTexture(p.format,p.width,p.height,video(SheetOffsets[index]));
}

inline unsigned animationLength(Graphic type) {
    const unsigned i=static_cast<unsigned>(type);
    return i<std::size(Packed::Graphics)?Packed::Graphics[i].count:0;
}

inline unsigned flyFrame(unsigned ticks) {
    return 1+((ticks/2)%2);
}

inline void drawFrame(unsigned index,float x,float y,float rotation,float scaleX,float scaleY,
                      uint32_t tint,bool flip) {
    if (!catalog) return;
    const auto& frame=Packed::Frames[index];
    const float factorX=scaleX*Scale*(flip?-1:1),factorY=scaleY*Scale;
    const float cosine=rotation?std::cos(rotation):1,sine=rotation?std::sin(rotation):0;
    const float centerX=ViewX+x*Scale,centerY=y*Scale;
    unsigned bound=SheetCount;
    for (unsigned i=0;i<frame.count;++i) {
        const auto& part=Packed::Parts[frame.first+i];
        TextureVertex quad[4];
        float minX=4096,maxX=-4096,minY=4096,maxY=-4096;
        for (unsigned corner=0;corner<4;++corner) {
            const bool right=(corner&1)!=0,bottom=(corner&2)!=0;
            const float dx=(part.x+(right?part.width:0))*factorX;
            const float dy=(part.y+(bottom?part.height:0))*factorY;
            const float outX=centerX+cosine*dx-sine*dy,outY=centerY+sine*dx+cosine*dy;
            minX=std::min(minX,outX); maxX=std::max(maxX,outX);
            minY=std::min(minY,outY); maxY=std::max(maxY,outY);
            quad[corner]={static_cast<float>(part.u+(right?part.w:0)),
                          static_cast<float>(part.v+(bottom?part.h:0)),tint,outX,outY,0};
        }
        if (maxX<ViewX || minX>ViewX+ViewRight || maxY<0 || minY>272) continue;
        if (bound!=part.page) {
            bindPage(part.page);
            bound=part.page;
        }
        auto* draw=static_cast<TextureVertex*>(sceGuGetMemory(sizeof quad));
        std::memcpy(draw,quad,sizeof quad);
        sceGuDrawArray(GU_TRIANGLE_STRIP,DisplayFx,4,nullptr,draw);
    }
}

inline void spriteScaled(Graphic type,unsigned frame,float x,float y,float rotation,float scaleX,
                         float scaleY,uint32_t tint=White,bool flip=false) {
    const unsigned value=static_cast<unsigned>(type);
    if (value>=std::size(Packed::Graphics)) return;
    const auto& art=Packed::Graphics[value];
    drawFrame(art.first+frame%art.count,x,y,rotation,scaleX,scaleY,tint,flip);
}

inline void sprite(Graphic type,unsigned frame,float x,float y,float rotation=0,float scale=1,
                   uint32_t tint=White,bool flip=false) {
    spriteScaled(type,frame,x,y,rotation,scale,scale,tint,flip);
}

inline void pole(float x,float opening) {
    sprite(Graphic::Pole,0,x,opening-288);
    sprite(Graphic::Pole,0,x,opening+288);
}

inline void gator(float x,float y,float attackProgress,float fallRotation=0) {
    const float progress=std::clamp(attackProgress,0.f,1.f);
    sprite(Graphic::Gator,0,x-24+64*progress,y-64+64*progress,
           .349066f+(1.5708f-.349066f)*progress+fallRotation);
}

inline void aim(float x,float y,float endX,float endY) {
    const float dx=endX-x,dy=endY-y, length=std::sqrt(dx*dx+dy*dy);
    if (length<1) return;
    const float angle=std::atan2(dy,dx), dirX=dx/length,dirY=dy/length;
    constexpr float spacing=16;
    for (float d=spacing*.5f;d<length;d+=spacing)
        sprite(Graphic::Dotted,0,x+d*dirX,y+d*dirY,angle,2.f/3.f,GU_RGBA(255,255,255,122));
}

inline void title(float scroll=0) {
    drawFrame(Packed::TitleMarkIndex,512-scroll,256,0,1,1,White,false);
}

inline void introTree(float scroll=0) {
    drawFrame(Packed::IntroTreeIndex,48-scroll,1176,0,1,1,White,false);
}

inline void death(unsigned alpha) {
    rectangle(ViewX+StageLeft,0,ViewRight-StageLeft,272,GU_RGBA(0,0,0,255));
    drawFrame(Packed::DeathIndex,600,328,0,1,1,
              GU_RGBA(255,255,255,std::min(255u,alpha)),false);
}

inline uint32_t hue(unsigned ticks) {
    const unsigned part=ticks%360/60;
    constexpr unsigned low=47,high=82;
    const unsigned blend=ticks%60*(high-low)/60;
    const unsigned inc=low+blend,dec=high-blend;
    switch (part) {
    case 0: return GU_RGBA(high,inc,low,255);
    case 1: return GU_RGBA(dec,high,low,255);
    case 2: return GU_RGBA(low,high,inc,255);
    case 3: return GU_RGBA(low,dec,high,255);
    case 4: return GU_RGBA(inc,low,high,255);
    default: return GU_RGBA(high,low,dec,255);
    }
}

inline void hud(unsigned ticks,float hp,bool showGameHud=true,bool russian=false) {
    fullView();
    rectangle(ViewX,0,StageLeft,272,GU_RGBA(0,0,0,255));
    drawFrame(Packed::HudBackdropIndex,124,276,0,1,1,hue(ticks),false);
    drawFrame(Packed::GameMarkIndex,124,20,0,1,1,White,false);
    drawFrame(Packed::HudMarkIndex,124,88,0,1,1,White,false);
    drawFrame(Packed::BorderIndex,252,256,0,1,1,White,false);
    if (!showGameHud) return;
    if (!russian) drawFrame(Packed::DistanceMarkIndex,128,216,0,1,1,White,false);
    drawFrame(Packed::HealthBackIndex,128,416,0,1,1,White,false);
    const float filled=std::clamp(hp,0.f,1.f);
    if (filled<=0) return;
    sceGuScissor(ViewX+4,212,ViewX+static_cast<int>((8+240*filled)*Scale),230);
    drawFrame(Packed::HealthFillIndex,128,416,0,1,1,White,false);
    fullView();
}

struct WorldTile { unsigned x,y,w,h,offset; };
inline constexpr WorldTile Tiles[]={
    {0,0,256,256,0},
    {256,0,16,256,2*256*256},
    {0,256,256,16,2*256*272},
    {256,256,16,16,2*(256*272+256*16)}
};

inline void warpedStrip(float u0,float u1,float v0,float v1,float x0,float x1,
                        float y0,float y1,float wave0,float wave1) {
    auto* quad=static_cast<TextureVertex*>(sceGuGetMemory(4*sizeof(TextureVertex)));
    quad[0]={u0,v0,White,x0,y0+wave0,0};
    quad[1]={u1,v0,White,x1,y0+wave1,0};
    quad[2]={u0,v1,White,x0,y1+wave0,0};
    quad[3]={u1,v1,White,x1,y1+wave1,0};
    sceGuDrawArray(GU_TRIANGLE_STRIP,DisplayFx,4,nullptr,quad);
}

inline void worldTile(const WorldTile& t,float startX,bool mirror,unsigned flash,float time) {
    const float left=startX+(mirror?272-t.x-t.w:t.x);
    if (left+t.w<=ViewX+StageLeft || left>=ViewX+ViewRight) return;
    setTexture(GU_PSM_5650,t.w,t.h,video(Start+t.offset));
    const auto textureX=[&](float screenX) {
        const float distance=screenX-left;
        return mirror?t.w-distance:distance;
    };
    const float stageX0=ViewX+StageLeft,stageX1=ViewX+ViewRight;
    if (!flash) {
        const float x0=std::max(left,stageX0),x1=std::min(left+t.w,stageX1);
        warpedStrip(textureX(x0),textureX(x1),0,t.h,x0,x1,t.y,t.y+t.h,0,0);
        return;
    }
    const float amplitude=-(272*.2f/255)*flash;
    for (unsigned i=0;i<t.w;i+=16) {
        const unsigned end=std::min(i+16,t.w);
        const float x0=std::max(left+i,stageX0),x1=std::min(left+end,stageX1);
        if (x0>=x1) continue;
        const float u0=textureX(x0),u1=textureX(x1);
        const float wave0=amplitude*std::cos(time*2+(t.x+u0)*(20.f/272));
        const float wave1=amplitude*std::cos(time*2+(t.x+u1)*(20.f/272));
        const float y0=t.y,y1=t.y+t.h;
        if (!t.y) warpedStrip(u0,u1,0,0,x0,x1,y0-56,y0,wave0,wave1);
        warpedStrip(u0,u1,0,t.h,x0,x1,y0,y1,wave0,wave1);
        if (t.y) warpedStrip(u0,u1,t.h-.5f,t.h-.5f,x0,x1,y1,y1+56,wave0,wave1);
    }
}

inline void background(float scroll,unsigned flash=0,float seconds=-1) {
    if (currentWorld==Backgrounds) return;
    sceGuTexFlush();
    fieldView();
    const float displacement=std::fmod(scroll,1024.f)*Scale;
    const float first=ViewX+StageLeft-displacement;
    const float time=seconds<0?scroll/20.f:seconds;
    for (int i=-1;i<=2;++i) {
        const float start=first+i*272;
        if (start+272<=ViewX+StageLeft || start>=ViewX+ViewRight) continue;
        for (const auto& tile:Tiles) worldTile(tile,start,(i%2)!=0,flash,time);
    }
    if (flash) {
        sceGuBlendFunc(GU_ADD,GU_SRC_ALPHA,GU_FIX,0,0xffffffff);
        rectangle(ViewX+StageLeft,0,ViewRight-StageLeft,272,
                  GU_RGBA(255,255,255,std::min(255u,flash)));
        sceGuBlendFunc(GU_ADD,GU_SRC_ALPHA,GU_ONE_MINUS_SRC_ALPHA,0,0);
    }
    fullView();
}

} // namespace MeowlsArt
