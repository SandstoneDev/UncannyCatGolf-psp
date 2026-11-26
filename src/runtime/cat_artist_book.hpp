#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>

namespace ArtistBook {

inline constexpr int Side=128;
inline constexpr unsigned Slots=3, Colors=20, Blank=24, Copycat=2;
using Canvas=std::array<uint8_t,Side*Side>;
using FillQueue=std::array<uint16_t,Side*Side>;
using BaseSlots=std::array<uint8_t,Slots>;

struct Book {
    unsigned generation=0,active=0;
    BaseSlots bases{0,1,Blank};
    std::array<Canvas,Slots> art{};
};

constexpr unsigned index(int x,int y) { return y*Side+x; }
constexpr bool inside(int x,int y) { return x>=0 && y>=0 && x<Side && y<Side; }

inline bool stroke(Canvas& pixels,int x,int y,int endX,int endY,int radius,uint8_t color) {
    int dx=std::abs(endX-x),dy=-std::abs(endY-y);
    const int sx=x<endX?1:-1,sy=y<endY?1:-1;
    int error=dx+dy;
    bool changed=false;
    for (;;) {
        for (int v=std::max(0,y-radius);v<=std::min(Side-1,y+radius);++v) {
            for (int u=std::max(0,x-radius);u<=std::min(Side-1,x+radius);++u) {
                auto& pixel=pixels[index(u,v)];
                changed|=pixel!=color;
                pixel=color;
            }
        }
        if (x==endX && y==endY) return changed;
        const int twice=2*error;
        if (twice>=dy) { error+=dy; x+=sx; }
        if (twice<=dx) { error+=dx; y+=sy; }
    }
}

inline bool flood(Canvas& canvas,int x,int y,uint8_t color,FillQueue& queue) {
    if (!inside(x,y)) return false;
    const auto previous=canvas[index(x,y)];
    if (previous==color) return false;
    unsigned first=0,last=0;
    const auto push=[&](unsigned pos) {
        if (canvas[pos]!=previous) return;
        canvas[pos]=color;
        queue[last++]=static_cast<uint16_t>(pos);
    };
    push(index(x,y));
    while (first<last) {
        const unsigned pos=queue[first++];
        if (pos%Side) push(pos-1);
        if (pos%Side!=Side-1) push(pos+1);
        if (pos>=Side) push(pos-Side);
        if (pos<Side*(Side-1)) push(pos+Side);
    }
    return true;
}

inline constexpr uint32_t Magic=0x31414355; // UCA1, Cat Artist book
struct Header {
    uint32_t magic=Magic,generation=0,checksum=0;
    uint8_t active=0;
    BaseSlots bases{0,1,Blank};
};
static_assert(sizeof(Header)==16);

inline uint32_t headerChecksum(const Header& h) {
    uint32_t hash=2166136261u;
    for (unsigned shift=0;shift<32;shift+=8) hash=(hash^((h.generation>>shift)&255))*16777619u;
    hash=(hash^h.active)*16777619u;
    for (const auto base:h.bases) hash=(hash^base)*16777619u;
    return hash;
}

inline uint32_t checksum(const Header& h,const Book& book) {
    uint32_t hash=headerChecksum(h);
    for (const auto& canvas:book.art)
        for (const auto pixel:canvas) hash=(hash^pixel)*16777619u;
    return hash;
}

inline bool readHeader(std::FILE* stream,Header& h) {
    return stream && std::fread(&h,sizeof h,1,stream)==1 && h.magic==Magic && h.generation &&
           h.active<Slots && std::all_of(h.bases.begin(),h.bases.end(),[](uint8_t base) {
               return base<=Blank && base!=Copycat;
           });
}

inline Header getHeader(const std::string& path) {
    auto* stream=std::fopen(path.c_str(),"rb");
    Header h{};
    if (!readHeader(stream,h)) h={};
    if (stream) std::fclose(stream);
    return h;
}

inline bool read(const std::string& path,Book& book) {
    auto* stream=std::fopen(path.c_str(),"rb");
    Header h{};
    const bool loaded=readHeader(stream,h) &&
                       std::fread(book.art.data(),sizeof book.art,1,stream)==1 &&
                       std::fgetc(stream)==EOF && !std::ferror(stream);
    if (stream) std::fclose(stream);
    if (!loaded || checksum(h,book)!=h.checksum) return false;
    for (const auto& canvas:book.art)
        if (std::any_of(canvas.begin(),canvas.end(),[](uint8_t pixel) { return pixel>Colors; })) return false;
    book.active=h.active;
    book.bases=h.bases;
    book.generation=h.generation;
    return true;
}

inline bool load(const std::string& prefix,Book& book) {
    const auto a=prefix+".0",b=prefix+".1";
    const auto h0=getHeader(a),h1=getHeader(b);
    const bool newer=h1.generation && (!h0.generation ||
                     static_cast<int32_t>(h1.generation-h0.generation)>0);
    if (newer?(read(b,book)||read(a,book)):(read(a,book)||read(b,book))) return true;
    book.generation=book.active=0;
    book.bases={0,1,Blank};
    for (auto& canvas:book.art) canvas.fill(0);
    return false;
}

inline bool write(const std::string& prefix,Book& book) {
    Header h{};
    h.generation=book.generation+1;
    if (!h.generation) h.generation=2;
    h.active=static_cast<uint8_t>(book.active);
    h.bases=book.bases;
    h.checksum=checksum(h,book);
    const auto path=prefix+((h.generation&1)?".1":".0");
    auto* stream=std::fopen(path.c_str(),"wb");
    if (!stream) return false;
    const bool wrote=std::fwrite(&h,sizeof h,1,stream)==1 &&
                      std::fwrite(book.art.data(),sizeof book.art,1,stream)==1;
    if (std::fclose(stream)!=0 || !wrote) return false;
    book.generation=h.generation;
    return true;
}

} // namespace ArtistBook
