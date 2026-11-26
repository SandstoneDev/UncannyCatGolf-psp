#pragma once

#include "asset_file.hpp"
#include "minigame_pause_layout060.hpp"
#include "dash_art_data.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <pspge.h>
#include <pspgu.h>
#include <zlib.h>

namespace DashArt {

inline constexpr float Scale=17.f/32;
inline constexpr unsigned Start=0x88000;
inline constexpr unsigned Count=std::size(Packed::Pages);
struct Record {uint32_t offset,bytes,raw,format;};
inline std::array<Record,Count> records{};
inline bool ready=false;
inline constexpr std::array<unsigned,Count+1> pageOffsets() {
    std::array<unsigned,Count+1> result{};
    result[0]=Start;
    for (unsigned i=0;i<Count;++i) result[i+1]=result[i]+Packed::Pages[i].bytes;
    return result;
}
inline constexpr auto Offsets=pageOffsets();
static_assert(Offsets.back()<=0x200000);

inline uint8_t* video(unsigned offset) {return static_cast<uint8_t*>(sceGeEdramGetAddr())+offset;}
inline void unload() {ready=false;}

inline bool inflatePage(FILE* file,const Record& record,unsigned destination) {
    if (std::fseek(file,record.offset,SEEK_SET)!=0) return false;
    z_stream stream{};
    if (inflateInit(&stream)!=Z_OK) return false;
    alignas(64) uint8_t input[4096],output[4096];
    unsigned written=0,remaining=record.bytes;
    auto* target=reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(video(destination))|0x40000000u);
    bool good=false;
    for (;;) {
        if (!stream.avail_in && remaining) {
            const unsigned n=std::min<unsigned>(sizeof input,remaining);
            if (std::fread(input,1,n,file)!=n) break;
            remaining-=n;
            stream.next_in=input;
            stream.avail_in=n;
        }
        stream.next_out=output;
        stream.avail_out=sizeof output;
        const int status=inflate(&stream,Z_NO_FLUSH);
        const unsigned n=sizeof output-stream.avail_out;
        if ((status!=Z_OK && status!=Z_STREAM_END) || n>record.raw-written) break;
        std::memcpy(target+written,output,n);
        written+=n;
        if (status==Z_STREAM_END) {
            good=written==record.raw && !remaining && !stream.avail_in;
            break;
        }
    }
    inflateEnd(&stream);
    return good;
}

inline bool load(const char* executable) {
    ready=false;
    if (static_cast<unsigned>(sceGeEdramGetSize())<Offsets.back()) return false;
    const auto path=companion(executable,"data/world/uncanny-dash-art.bin");
    const auto input=openAssetFile(path.c_str());
    uint32_t header[2]{};
    if (!input || std::fread(header,sizeof header,1,input.get())!=1 || header[0]!=0x41444355 ||
        header[1]!=Count || std::fread(records.data(),sizeof records,1,input.get())!=1 ||
        std::fseek(input.get(),0,SEEK_END)!=0) return false;
    const auto length=std::ftell(input.get());
    if (length<0) return false;
    unsigned end=sizeof header+sizeof records;
    for (unsigned i=0;i<Count;++i) {
        const auto& record=records[i];
        if (!record.bytes || record.offset!=end || record.raw!=Packed::Pages[i].bytes ||
            record.format!=Packed::Pages[i].format || record.offset>static_cast<unsigned long>(length) ||
            record.bytes>static_cast<unsigned long>(length)-record.offset) return false;
        end+=record.bytes;
    }
    if (end!=static_cast<unsigned long>(length)) return false;
    for (unsigned i=0;i<Count;++i)
        if (!inflatePage(input.get(),records[i],Offsets[i])) return false;
    sceGuTexFlush();
    ready=true;
    return true;
}

inline void fieldView() {sceGuScissor(ViewX+136,0,ViewX+408,272);}
inline void fullView() {sceGuScissor(ViewX,0,ViewX+408,272);}

inline void frame(unsigned index,float x,float y,float scaleX=1,float scaleY=1,
                  float angle=0,uint32_t tint=White) {
    if (!ready || index>=std::size(Packed::Frames)) return;
    const auto& image=Packed::Frames[index];
    const float sine=std::sin(angle),cosine=std::cos(angle);
    sceGuTexFilter(GU_NEAREST,GU_NEAREST);
    for (unsigned i=image.start;i<image.start+image.count;++i) {
        const auto& p=Packed::Parts[i];
        setTexture(Packed::Pages[p.page].format,256,256,video(Offsets[p.page]));
        auto* vertices=static_cast<TextureVertex*>(sceGuGetMemory(4*sizeof(TextureVertex)));
        for (unsigned corner=0;corner<4;++corner) {
            const float dx=(p.x+(corner&1?p.w:0))*scaleX,dy=(p.y+(corner&2?p.h:0))*scaleY;
            vertices[corner]={float(p.u+(corner&1?p.width:0)),float(p.v+(corner&2?p.height:0)),tint,
                              ViewX+(x+dx*cosine-dy*sine)*Scale,(y+dx*sine+dy*cosine)*Scale,0};
        }
        sceGuDrawArray(GU_TRIANGLE_STRIP,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,
                       4,nullptr,vertices);
    }
}

inline void drawBackdrop(unsigned ticks) {
    fieldView();
    const float time=(ticks%600)/60.f;
    const float zoom=time<5?1+time*.1f:2-time*.1f;
    frame(Packed::Background,512,256,zoom,zoom,time*.628319f);
    constexpr float pulseTimes[]={0,2.5f,5.03333f,7.5f,10};
    constexpr float red[]={1,.418f,1,.418f,1},other[]={1,.55f,1,.55f,1};
    unsigned segment=0;
    while (segment<3 && time>=pulseTimes[segment+1]) ++segment;
    const float t=(time-pulseTimes[segment])/(pulseTimes[segment+1]-pulseTimes[segment]);
    const unsigned r=static_cast<unsigned>(255*(red[segment]+(red[segment+1]-red[segment])*t));
    const unsigned gb=static_cast<unsigned>(255*(other[segment]+(other[segment+1]-other[segment])*t));
    frame(Packed::Hints,512,256,1,1,0,GU_RGBA(r,gb,gb,255));
}
inline void drawHud() {
    fullView();
    frame(Packed::Sidebar,124,276);
    frame(Packed::Logo,124,20);
    frame(Packed::Border,252,256);
}
inline void tile(bool land,unsigned parity,float x,float y,float sx=1,float sy=1,uint32_t tint=White) {
    fieldView();
    frame(Packed::Tile00+(parity%2)*2+(land?1:0),x,y,sx,sy,0,tint);
}
inline void cat(float x,float y,float sx,float sy,uint32_t tint) {
    fieldView();
    frame(Packed::Tile11,x,y,sx,sy,0,tint);
}
inline void digit(unsigned value,float x,float y,float scale=1) {
    fieldView();
    frame(Packed::Digit0+value%10,x,y,scale,scale);
}
inline void jumpscare() {fieldView();frame(Packed::Scare,512,256);}
inline void cursor(float x,float y) {fullView();frame(Packed::Cursor,x,y);}
inline void pause(bool ru,int hovered=-1,int pressed=-1) {
    fieldView();
    rectangle(ViewX+136,0,272,272,0xff000000u);
    const unsigned first=ru?Packed::Pause1header0:Packed::Pause0header0;
    const auto label=[](unsigned index,unsigned row){
        const auto& p=Packed::Parts[Packed::Frames[index].start];
        constexpr float k=MinigamePauseLayout060::Scale;
        frame(index,MinigamePauseLayout060::left(p.w*Scale)/Scale-p.x*k,
              MinigamePauseLayout060::top(p.h*Scale,row)/Scale-p.y*k,k,k);
    };
    label(first,0);
    for (int row=0;row<2;++row) {
        const unsigned state=pressed==row?2u:hovered==row?1u:0u;
        label(first+1+row*3+state,row+1);
    }
}

} // namespace DashArt
