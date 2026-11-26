#pragma once

#include "asset_file.hpp"
#include "minigame_pause_layout060.hpp"
#include "bort_art_data.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <pspge.h>
#include <pspkernel.h>
#include <pspgu.h>
#include <zlib.h>

namespace BortArt {

inline constexpr float Scale=17.f/32;
inline constexpr unsigned Start=0x88000;
inline constexpr unsigned Count=std::size(Packed::Pages);
struct Record {uint32_t offset,bytes,raw,format;};
inline std::array<Record,Count> records{};
inline bool ready=false,textureDirty=false;
inline constexpr std::array<unsigned,Count+1> pageOffsets() {
    std::array<unsigned,Count+1> result{};
    result[0]=Start;
    for (unsigned i=0;i<Count;++i) result[i+1]=result[i]+Packed::Pages[i].bytes;
    return result;
}
inline constexpr auto Offsets=pageOffsets();
static_assert(Offsets.back()<=0x200000);

inline uint8_t* video(unsigned offset) {return static_cast<uint8_t*>(sceGeEdramGetAddr())+offset;}
inline void unload() {ready=false;textureDirty=false;}

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
    ready=false;textureDirty=false;
    if (static_cast<unsigned>(sceGeEdramGetSize())<Offsets.back()) return false;
    const auto path=companion(executable,"data/world/bort-bash-art.bin");
    const auto input=openAssetFile(path.c_str());
    uint32_t header[2]{};
    if (!input || std::fread(header,sizeof header,1,input.get())!=1 || header[0]!=0x41424355 ||
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
    // The arena may still be sampled by the previous scene on real hardware.
    sceGuSync(GU_SYNC_FINISH,GU_SYNC_WHAT_DONE);
    for (unsigned i=0;i<Count;++i)
        if (!inflatePage(input.get(),records[i],Offsets[i])) return false;
    textureDirty=true;
    ready=true;
    return true;
}

inline void fieldView() {sceGuScissor(ViewX+136,34,ViewX+408,272);}
inline void fullView() {sceGuScissor(ViewX,0,ViewX+408,272);}

inline void frame(unsigned index,float x,float y,float scaleX=1,float scaleY=1,
                  float angle=0,uint32_t tint=White) {
    if (!ready || index>=std::size(Packed::Frames)) return;
    if (textureDirty) {sceGuTexFlush();textureDirty=false;}
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

// Each palette lives in this draw's display-list memory until the GE completes it.
// No shared palette is overwritten while earlier Borts are in flight.
inline uint32_t hsv(float hue,float saturation,float value) {
    hue-=std::floor(hue);
    const float h=hue*6.f;
    const int sector=static_cast<int>(h);
    const float f=h-sector,p=value*(1-saturation),q=value*(1-saturation*f),t=value*(1-saturation*(1-f));
    float r=0,g=0,b=0;
    switch (sector) {
        case 0:r=value;g=t;b=p;break;
        case 1:r=q;g=value;b=p;break;
        case 2:r=p;g=value;b=t;break;
        case 3:r=p;g=q;b=value;break;
        case 4:r=t;g=p;b=value;break;
        default:r=value;g=p;b=q;break;
    }
    return GU_RGBA(static_cast<unsigned>(r*255+.5f),static_cast<unsigned>(g*255+.5f),static_cast<unsigned>(b*255+.5f),255);
}
inline void bort(float x,float y,unsigned animationFrame,float hue) {
    if (!ready) return;
    fieldView();
    auto* storage=static_cast<uint8_t*>(sceGuGetMemory(16*sizeof(uint32_t)+63));
    auto* palette=reinterpret_cast<uint32_t*>((reinterpret_cast<uintptr_t>(storage)+63)&~uintptr_t(63));
    std::fill_n(palette,16,0u);
    palette[1]=0xffffffffu;
    palette[2]=hsv(hue,.5f,1.f);
    palette[3]=0xff000000u;
    palette[4]=hsv(hue,1.f,.5f);
    sceKernelDcacheWritebackRange(palette,16*sizeof(uint32_t));
    sceGuClutMode(GU_PSM_8888,0,15,0);
    sceGuClutLoad(2,palette);
    frame(Packed::Bort0+animationFrame%2,x,y);
}
inline void explosion(float x,float y,unsigned animationFrame) {
    fieldView();
    frame(Packed::Explosion0+std::min(animationFrame,16u),x,y);
}
inline unsigned numberDigits(unsigned value,unsigned (&digits)[10]) {
    unsigned count=0;
    do {digits[count++]=value%10;value/=10;} while(value && count<10);
    return count;
}
inline float numberWidth(unsigned style,unsigned value) {
    unsigned digits[10];
    const unsigned count=numberDigits(value,digits);
    float result=0;
    for (unsigned i=count;i>0;--i) {
        result+=Packed::Advances[style][digits[i-1]];
        if (i>1) result+=Packed::Kerns[style][digits[i-1]][digits[i-2]];
    }
    return result;
}
inline void number(unsigned style,unsigned value,float x,float y) {
    unsigned digits[10];
    const unsigned count=numberDigits(value,digits);
    const unsigned first[2][4]={
        {Packed::Shadow0_0,Packed::Shadow1_0,Packed::Shadow2_0,Packed::Shadow3_0},
        {Packed::Face0_0,Packed::Face1_0,Packed::Face2_0,Packed::Face3_0}};
    // Godot draws all text shadows before the foreground glyphs.
    for (unsigned layer=0;layer<2;++layer) {
        float position=x;
        for (unsigned i=count;i>0;--i) {
            const unsigned digit=digits[i-1];
            frame(first[layer][style]+digit,position,y);
            position+=Packed::Advances[style][digit];
            if (i>1) position+=Packed::Kerns[style][digit][digits[i-2]];
        }
    }
}
inline void drawScene(unsigned round,unsigned left,unsigned score,float timerSeconds,bool ru) {
    fullView();
    frame(Packed::Background,512,288);
    frame(Packed::Sidebar,0,0);
    frame(Packed::Header,512,32);
    const unsigned values[]={round,left,score};
    const unsigned prefixes[]={Packed::Prefix0_0,Packed::Prefix1_0,Packed::Prefix2_0};
    for (unsigned style=0;style<3;++style) {
        const float y=128.f+style*64;
        frame(prefixes[style]+static_cast<unsigned>(ru),16,y);
        unsigned leading=values[style];
        while (leading>=10) leading/=10;
        number(style,values[style],16+Packed::PrefixWidths[style][ru]+Packed::PrefixKerns[style][ru][leading],y);
    }
    const unsigned seconds=static_cast<unsigned>(std::ceil(std::max(0.f,std::min(timerSeconds,999999.f))));
    number(3,seconds,512-std::ceil(numberWidth(3,seconds))*.5f,0);
}
inline void prompt(bool ru,bool hovered=false,bool pressed=false) {
    fullView();
    frame(Packed::PromptPanel,352,208);
    frame(Packed::Prompt0+static_cast<unsigned>(ru),320,208);
    frame(Packed::Start0+(pressed?2u:hovered?1u:0u),448,312);
}
inline void gameOver(unsigned rank,bool ru,bool hovered=false,bool pressed=false) {
    fullView();
    rectangle(ViewX+320*Scale,128*Scale,384*Scale,320*Scale,0xb9000000u);
    frame(Packed::Over0+static_cast<unsigned>(ru),320,128);
    frame(Packed::Status0+static_cast<unsigned>(ru),504,192);
    frame(Packed::Retry0+static_cast<unsigned>(ru),344,200);
    frame(Packed::Rank0+std::min(rank,6u),600,320);
    frame(Packed::Exit0+(pressed?2u:hovered?1u:0u),360,384);
}
inline void cursor(float x,float y) {fullView();frame(Packed::Cursor,x,y);}
inline void pause(bool ru,int hovered=-1,int pressed=-1) {
    fullView();
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
} // namespace BortArt
