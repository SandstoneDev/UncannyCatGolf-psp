#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>

namespace EditorSpritePrefixMemo {
using Words = std::array<uint32_t,25>;
inline uint32_t hashContext(const Words& words) {
    uint32_t hash=2166136261u;
    for(uint32_t word:words){hash^=word;hash*=16777619u;}
    return hash;
}
struct Context { Words words{};uint32_t hash=0;bool admissible=false; };
inline bool finiteRange(double value) {
    static_assert(sizeof(double)==8,"binary64 operands");
    uint64_t bits;std::memcpy(&bits,&value,sizeof bits);
    return (bits&UINT64_C(0x7fffffffffffffff))<=UINT64_C(0x4270000000000000);
}
inline bool positive(double value) {
    uint64_t bits;std::memcpy(&bits,&value,sizeof bits);
    return !(bits>>63)&&(bits<<1)!=0;
}
template<class Region,class Sprite,class Point>
Context makeContext(unsigned spriteId,unsigned image,unsigned frame,const Region& region,
                    double fw,double fh,const Sprite& s,Point scale,double rotation,
                    bool nearestEven) {
    Context context;unsigned at=0;bool valid=nearestEven;
    auto integer=[&](uint32_t value){context.words[at++]=value;};
    auto floating=[&](float value){
        uint32_t bits;std::memcpy(&bits,&value,sizeof bits);integer(bits);
        valid=valid&&((bits&0x7fffffffu)<=0x53800000u);
    };
    auto wide=[&](double value){
        std::memcpy(context.words.data()+at,&value,sizeof value);at+=2;
        valid=valid&&finiteRange(value);
    };
    integer(spriteId);integer(image);integer(frame);
    floating(region.x);floating(region.y);floating(region.w);floating(region.h);
    wide(fw);wide(fh);
    floating(s.x);floating(s.y);floating(s.sx);floating(s.sy);
    floating(s.rotation);floating(s.ox);floating(s.oy);integer(s.centered?1u:0u);
    wide(scale.x);wide(scale.y);wide(rotation);
    context.admissible=valid&&positive(fw)&&positive(fh)&&positive(double(region.w))&&positive(double(region.h));
    context.hash=hashContext(context.words);
    return context;
}

// Stores only the original local Vec2 before entity position/View projection.
// Every scalar operand and source identity is compared, without padding bytes.
// Direct-map collisions recompute. Provider tables are immutable; caller must
// clear at their lifecycle boundary. Non-nearest/unknown inputs always compute.
template<class Point,unsigned Capacity=64> class Cache {
    static_assert(Capacity&&!(Capacity&(Capacity-1)),"power-of-two bounded slots");
    static_assert(std::is_trivially_copyable<Point>::value&&sizeof(Point)==16,"full binary64 Vec2 copies");
    struct Entry { Words words{};std::array<uint32_t,4> pixel{};Point value{};bool valid=false; };
    std::array<Entry,Capacity> entries{};
public:
    static constexpr size_t fixedBytes(){return sizeof(Cache);}
    void clear(){for(auto& entry:entries)entry.valid=false;}
    template<class Compute> Point get(const Context& context,Point pixel,const Compute& compute){
        if(!context.admissible||!finiteRange(pixel.x)||!finiteRange(pixel.y))return compute();
        static_assert(sizeof(pixel.x)==8&&sizeof(pixel.y)==8,"binary64 coordinates");
        std::array<uint32_t,4> bits;
        std::memcpy(bits.data(),&pixel.x,8);std::memcpy(bits.data()+2,&pixel.y,8);
        uint32_t hash=context.hash;
        for(uint32_t word:bits){hash^=word;hash*=16777619u;}
        hash^=hash>>16;
        auto& entry=entries[hash&(Capacity-1)];
        if(entry.valid&&entry.words==context.words&&entry.pixel==bits)return entry.value;
        const Point value=compute();
        if(finiteRange(value.x)&&finiteRange(value.y)){
            entry.words=context.words;entry.pixel=bits;entry.value=value;entry.valid=true;
        }
        return value;
    }
};
}
