#pragma once

#include "asset_file.hpp"

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <iterator>
#ifdef __PSP__
#include <malloc.h>
#endif
#include <pspgu.h>
#include <pspkernel.h>
#include <pspge.h>
#include <zlib.h>

namespace CatArtistArt {
enum class Berry { Dance, Idle, PointDown, PointUp, Sad, Talk, Wave };
struct Stop { unsigned position; unsigned channel[3]; };
}

#include "cat_artist_authored.hpp"

namespace CatArtistArt {

inline constexpr unsigned UiOffset=0x110000, CostumeOffset=0x168000, ArenaEnd=0x1f0000;
inline constexpr unsigned WoodBytes=256*256*2+256*16*2;
inline constexpr unsigned ToolsBytes=128*128*4;
inline constexpr unsigned BerryPaletteBytes=16*4, BerryPageBytes=256*256, BerrySmallBytes=128*128;
inline constexpr unsigned SpeechBytes=128*128*2;
inline constexpr unsigned UiBytes=WoodBytes+ToolsBytes+BerryPaletteBytes+BerryPageBytes+BerrySmallBytes+SpeechBytes;
inline constexpr unsigned CostumeBytes=272*272*4, PreviewBytes=32*32*4;
inline constexpr unsigned CostumeCount=25, PreviewCount=3, NoCostume=CostumeCount, Rainbow=8;
static_assert(UiOffset+UiBytes<=CostumeOffset);
static_assert(CostumeOffset+CostumeBytes+PreviewCount*PreviewBytes<=ArenaEnd);

struct Entry { uint32_t offset,bytes,previewOffset,previewBytes; };

inline AssetFile costumes;
inline Entry entries[CostumeCount]{};
alignas(64) inline uint32_t berryColors[16]{};
alignas(64) inline uint32_t berryClut[16]{};
inline unsigned currentCostume=NoCostume;
inline unsigned previews[PreviewCount]={NoCostume,NoCostume,NoCostume};
inline bool loaded=false;
inline bool textureDirty=false;

struct LoadFailure {
    const char* stage="none";
    long expected=0,actual=0;
    int error=0,fileError=0,zlib=Z_OK;
    unsigned heapUsed=0,heapFree=0,partitionFree=0,partitionLargest=0;

    bool record(const char* where,long want,long got,FILE* file=nullptr,int status=Z_OK,int savedErrno=errno) {
        stage=where;
        expected=want;
        actual=got;
        error=savedErrno;
        fileError=file?std::ferror(file):0;
        zlib=status;
#ifdef __PSP__
        const auto heap=mallinfo();
        heapUsed=static_cast<unsigned>(heap.uordblks);
        heapFree=static_cast<unsigned>(heap.fordblks);
        partitionFree=sceKernelTotalFreeMemSize();
        partitionLargest=sceKernelMaxFreeMemSize();
#endif
        return false;
    }
};

inline auto video(unsigned offset) {
    return static_cast<uint8_t*>(sceGeEdramGetAddr())+offset;
}

inline auto uncached(uint8_t* address) {
    return reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(address)|0x40000000u);
}

inline bool inflateToVideo(FILE* file,unsigned offset,unsigned bytes,uint8_t* videoMemory,unsigned size,
                           LoadFailure* failure=nullptr) {
    const auto fail=[&](const char* step,long expected,long actual,int status=Z_OK) {
        return failure && failure->record(step,expected,actual,file,status);
    };
    if (!bytes) return fail("inflate_empty",1,0);
    errno=0;
    if (const int result=std::fseek(file,offset,SEEK_SET);result!=0) return fail("inflate_seek",0,result);
    z_stream stream{};
    errno=0;
    if (const int result=inflateInit(&stream);result!=Z_OK) return fail("inflate_init",Z_OK,result,result);
    alignas(64) uint8_t input[4096],output[4096];
    unsigned written=0;
    auto* destination=uncached(videoMemory);
    bool ok=false;
    for (;;) {
        if (!stream.avail_in && bytes) {
            const unsigned count=std::min<unsigned>(sizeof input,bytes);
            errno=0;
            if (const auto got=std::fread(input,1,count,file);got!=count) {
                fail("inflate_read",count,got);
                break;
            }
            stream.next_in=input;
            stream.avail_in=count;
            bytes-=count;
        }
        stream.next_out=output;
        stream.avail_out=sizeof output;
        errno=0;
        const int status=inflate(&stream,Z_NO_FLUSH);
        const unsigned count=sizeof output-stream.avail_out;
        if (status!=Z_OK && status!=Z_STREAM_END) {
            fail("inflate_step",size,written+count,status);
            break;
        }
        if (count>size-written) {
            fail("inflate_output_limit",size,written+count,status);
            break;
        }
        std::memcpy(destination+written,output,count);
        written+=count;
        if (status==Z_STREAM_END) {
            ok=written==size && !bytes && !stream.avail_in;
            if (written!=size) fail("inflate_output_size",size,written,status);
            else if (!ok) fail("inflate_trailing_input",0,bytes+stream.avail_in,status);
            break;
        }
    }
    inflateEnd(&stream);
    return ok;
}

inline void unload() {
    costumes.reset();
    loaded=false;
    textureDirty=false;
    currentCostume=NoCostume;
    std::fill_n(previews,PreviewCount,NoCostume);
}

inline bool load(const char* executable,LoadFailure& failure) {
    if (loaded) return true;
    const auto vramSize=static_cast<unsigned>(sceGeEdramGetSize());
    if (vramSize<ArenaEnd) return failure.record("edram_size",ArenaEnd,vramSize,nullptr,Z_OK,0);
    errno=0;
    const auto ui=openAssetFile(companion(executable,"data/world/cat-artist-art.bin").c_str());
    const int uiError=errno;
    errno=0;
    auto catalog=openAssetFile(companion(executable,"data/world/cat-artist-costumes.bin").c_str());
    const int catalogError=errno;
    uint32_t size=0,count=0;
    char magic[4]{};
    const auto read=[&](const char* stage,FILE* file,void* output,size_t bytes,size_t items) {
        errno=0;
        const auto got=std::fread(output,bytes,items,file);
        return got==items || failure.record(stage,items,got,file);
    };
    const auto seekEnd=[&](const char* stage,FILE* file) {
        errno=0;
        const int result=std::fseek(file,0,SEEK_END);
        return result==0 || failure.record(stage,0,result,file);
    };
    const auto lengthIs=[&](const char* stage,FILE* file,long expected) {
        errno=0;
        const long length=std::ftell(file);
        return length==expected || failure.record(stage,expected,length,file);
    };

    if (!ui) return failure.record("ui_open",1,0,nullptr,Z_OK,uiError);
    if (!read("ui_magic_read",ui.get(),magic,sizeof magic,1)) return false;
    if (std::memcmp(magic,"CAU1",4)!=0) return failure.record("ui_magic",1,0,ui.get(),Z_OK,0);
    if (!read("ui_size_read",ui.get(),&size,sizeof size,1) || !seekEnd("ui_seek_end",ui.get()) ||
        !lengthIs("ui_file_size",ui.get(),static_cast<long>(8+size))) return false;
    if (!catalog) return failure.record("catalog_open",1,0,nullptr,Z_OK,catalogError);
    if (!read("catalog_magic_read",catalog.get(),magic,sizeof magic,1)) return false;
    if (std::memcmp(magic,"CAC1",4)!=0) return failure.record("catalog_magic",1,0,catalog.get(),Z_OK,0);
    if (!read("catalog_count_read",catalog.get(),&count,sizeof count,1)) return false;
    if (count!=CostumeCount) return failure.record("catalog_count",CostumeCount,count,catalog.get(),Z_OK,0);
    if (!read("catalog_entries_read",catalog.get(),entries,sizeof(Entry),CostumeCount) ||
        !seekEnd("catalog_seek_end",catalog.get())) return false;
    unsigned end=8+sizeof entries;
    for (const auto& entry:entries) {
        if (!entry.bytes || !entry.previewBytes)
            return failure.record("catalog_entry_bytes",1,0,catalog.get(),Z_OK,0);
        if (entry.offset!=end) return failure.record("catalog_entry_offset",end,entry.offset,catalog.get(),Z_OK,0);
        if (entry.previewOffset!=entry.offset+entry.bytes)
            return failure.record("catalog_preview_offset",entry.offset+entry.bytes,entry.previewOffset,catalog.get(),Z_OK,0);
        end=entry.previewOffset+entry.previewBytes;
    }
    auto* pixels=video(UiOffset);
    if (!lengthIs("catalog_file_size",catalog.get(),static_cast<long>(end)) ||
        !inflateToVideo(ui.get(),8,size,pixels,UiBytes,&failure)) return false;
    std::memcpy(berryColors,uncached(pixels+WoodBytes+ToolsBytes),BerryPaletteBytes);
    costumes=std::move(catalog);
    loaded=true;
    textureDirty=true;
    return true;
}

inline bool loadCostume(unsigned base,LoadFailure* failure=nullptr) {
    if (!loaded || base>=CostumeCount)
        return failure && failure->record("costume_selector",CostumeCount,base,nullptr,Z_OK,0);
    if (base==currentCostume) return true;
    const auto& entry=entries[base];
    currentCostume=NoCostume;
    if (!inflateToVideo(costumes.get(),entry.offset,entry.bytes,video(CostumeOffset),CostumeBytes,failure)) return false;
    currentCostume=base;
    textureDirty=true;
    return true;
}

inline bool loadPreview(unsigned slot,unsigned base) {
    if (!loaded || slot>=PreviewCount || base>=CostumeCount) return false;
    if (previews[slot]==base) return true;
    const auto& entry=entries[base];
    previews[slot]=NoCostume;
    if (!inflateToVideo(costumes.get(),entry.previewOffset,entry.previewBytes,
                        video(CostumeOffset+CostumeBytes+slot*PreviewBytes),PreviewBytes)) return false;
    previews[slot]=base;
    textureDirty=true;
    return true;
}

inline void bindTexture(int format,int w,int h,const uint8_t* pixels) {
    if (textureDirty) {
        sceGuTexFlush();
        textureDirty=false;
    }
    setTexture(format,w,h,pixels);
}

inline void sprite(const Patch& uv,float x,float y,float w,float h,uint32_t tint=White,bool flip=false) {
    patchScaled(uv,x,y,w/uv.width,h/uv.height,tint,flip);
}

inline void drawSidebar(float x,float y,float w,float h,bool /* russian */) {
    if (!loaded) return;
    const float top=h*(256.f/272.f);
    bindTexture(GU_PSM_5650,256,256,video(UiOffset));
    sprite({0,0,136,256,0,0},x,y,w,top);
    bindTexture(GU_PSM_5650,256,16,video(UiOffset+256*256*2));
    sprite({0,0,136,16,0,0},x,y+top,w,h-top);
}

inline void tool(const Patch& uv,float x,float y,float w,float h,uint32_t tint=White,bool flip=false) {
    if (!loaded) return;
    bindTexture(GU_PSM_8888,128,128,video(UiOffset+WoodBytes));
    sprite(uv,x,y,w,h,tint,flip);
}

inline void drawBrush(unsigned index,bool selected,float x,float y,float w,float h) {
    using namespace Authored;
    constexpr Patch icons[4][2]={{Brush00,Brush01},{Brush10,Brush11},{Brush20,Brush21},{Brush30,Brush31}};
    if (index<4) tool(icons[index][selected],x,y,w,h);
}

inline void drawMode(bool erase,float x,float y,float w,float h) {
    tool(erase?Authored::Tool1:Authored::Tool0,x,y,w,h);
}

inline void drawFill(bool enabled,float x,float y,float w,float h) {
    tool(enabled?Authored::Tool3:Authored::Tool2,x,y,w,h);
}

inline void drawArrow(bool left,float x,float y,float w,float h) {
    tool(Authored::Arrow,x,y,w,h,White,left);
}

inline void drawCursor(float x,float y,float w,float h) {
    tool(Authored::Cursor,x,y,w,h);
}

inline void drawPlank(float x,float y,float w,float h,uint32_t tint=White) {
    tool(Authored::Plank,x,y,w,h,tint);
}

inline void drawSpeech(float x,float y,float w,float h) {
    if (!loaded) return;
    bindTexture(GU_PSM_4444,128,128,video(UiOffset+WoodBytes+ToolsBytes+BerryPaletteBytes+BerryPageBytes+BerrySmallBytes));
    sprite({0,0,128,100,0,0},x,y,w,h);
}

template<unsigned Count>
inline uint32_t gradient(const Stop (&stops)[Count],float p,bool cubic=true) {
    const unsigned t=static_cast<unsigned>((p-std::floor(p))*65536);
    unsigned next=1;
    while (next+1<Count && t>stops[next].position) ++next;
    const auto& a=stops[next-1];
    const auto& b=stops[next];
    const float mix=static_cast<float>(t-a.position)/(b.position-a.position);
    float rgb[3]{};
    for (unsigned i=0;i<3;++i) {
        const float av=a.channel[i], bv=b.channel[i];
        if (cubic) {
            const float p0=stops[next>1?next-2:0].channel[i];
            const float p3=stops[next+1<Count?next+1:next].channel[i];
            const float m2=mix*mix, m3=m2*mix;
            rgb[i]=.5f*((2*av)+(-p0+bv)*mix+(2*p0-5*av+4*bv-p3)*m2+(-p0+3*av-3*bv+p3)*m3);
        } else rgb[i]=av+(bv-av)*mix;
        rgb[i]=std::clamp(rgb[i],0.f,255.f);
    }
    return GU_RGBA(static_cast<unsigned>(rgb[0]),static_cast<unsigned>(rgb[1]),static_cast<unsigned>(rgb[2]),255);
}

inline void drawBerry(Berry pose,unsigned ticks,float cx,float cy,float w,float h,bool flip=false) {
    if (!loaded) return;
    constexpr unsigned first[]={0,2,5,8,11,14,17};
    const auto kind=static_cast<unsigned>(pose);
    if (kind>=std::size(first)) return;
    const unsigned index=first[kind]+(kind==0?ticks/20%2:ticks/5%3);
    const unsigned number=index<16?index:index-16;
    const unsigned side=index<16?256:128;
    const unsigned columns=side/64;
    const auto u=static_cast<int16_t>(number%columns*64);
    const auto v=static_cast<int16_t>(number/columns*64);
    std::memcpy(berryClut,berryColors,sizeof berryColors);
    berryClut[Authored::BerryMagenta]=gradient(Authored::BerryGradient,(ticks%240)/240.f);
    sceKernelDcacheWritebackRange(berryClut,sizeof berryClut);
    sceGuClutMode(GU_PSM_8888,0,15,0);
    sceGuClutLoad(2,berryClut);
    bindTexture(GU_PSM_T8,side,side,video(UiOffset+WoodBytes+ToolsBytes+BerryPaletteBytes+(index<16?0:BerryPageBytes)));
    sprite({u,v,51,51,0,0},cx-w*.5f,cy-h*.5f,w,h,White,flip);
}

inline void costumeTile(const uint8_t* pixels,int wide,int high,float x,float y,float w,float h,
                        unsigned ticks,bool rainbow) {
    bindTexture(GU_PSM_8888,wide,high,pixels);
    if (!rainbow) {
        sprite({0,0,static_cast<int16_t>(wide),static_cast<int16_t>(high),0,0},x,y,w,h);
        return;
    }
    constexpr float viewport=408.f;
    const float step=w/wide, phase=static_cast<float>(ticks%120)/120.f;
    auto sample=[&](int column) {
        const float uv=(x+step*(column+.5f)-ViewX)/viewport-phase;
        return static_cast<unsigned>(std::floor((uv-std::floor(uv))*64));
    };
    for (int left=0;left<wide;) {
        const auto color=sample(left);
        int right=left+1;
        while (right<wide && sample(right)==color) ++right;
        sprite({static_cast<int16_t>(left),0,static_cast<int16_t>(right-left),static_cast<int16_t>(high),0,0},
               x+left*step,y,(right-left)*step,h,gradient(Authored::CostumeGradient,color/64.f,false));
        left=right;
    }
}

inline void drawCostume(float x,float y,float w,float h,unsigned ticks) {
    if (currentCostume>=CostumeCount) return;
    const float columns=w*(256.f/272.f), rows=h*(256.f/272.f);
    const auto* pixels=video(CostumeOffset);
    const bool rainbow=currentCostume==Rainbow;
    costumeTile(pixels,256,256,x,y,columns,rows,ticks,rainbow);
    costumeTile(pixels+256*256*4,16,256,x+columns,y,w-columns,rows,ticks,rainbow);
    costumeTile(pixels+(256*256+16*256)*4,256,16,x,y+rows,columns,h-rows,ticks,rainbow);
    costumeTile(pixels+(256*256+2*16*256)*4,16,16,x+columns,y+rows,w-columns,h-rows,ticks,rainbow);
}

inline void drawPreview(unsigned slot,float x,float y,float w,float h,unsigned ticks) {
    if (slot>=PreviewCount || previews[slot]>=CostumeCount) return;
    costumeTile(video(CostumeOffset+CostumeBytes+slot*PreviewBytes),32,32,x,y,w,h,ticks,previews[slot]==Rainbow);
}

} // namespace CatArtistArt
