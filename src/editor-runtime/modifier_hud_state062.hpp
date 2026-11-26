#pragma once
#ifndef UCG_MODIFIER_HUD_STATE062_DEFINED
#define UCG_MODIFIER_HUD_STATE062_DEFINED
#include <cstdint>
namespace ModifierHud061 {
enum class Context {None,Selector,Campaign,Endless,Editor};
constexpr uint16_t AllIcons=0x7fff;
// Canonical slots preserve RC9 campaign UVs. Order follows Modifiers.mods.
constexpr unsigned Order[]={0,1,2,3,4,5,6,7,8,11,12,9,13,14,10};
inline uint16_t fromEndless(unsigned mask){
    constexpr unsigned slots[]={1,6,7,8,9,12,13,14,11};
    uint16_t result=0;for(unsigned i=0;i<9;++i)if(mask&(1u<<i))result|=1u<<slots[i];return result;
}
inline uint16_t fromEditor(unsigned mask){
    constexpr unsigned slots[]={1,2,3,4,5,6,7,8,9,10,12,13,0};
    uint16_t result=0;for(unsigned i=0;i<13;++i)if(mask&(1u<<i))result|=1u<<slots[i];return result;
}
struct State {
    Context context=Context::None;
    unsigned profile=0;
    uint16_t mask=0;
    uint64_t started=0;
    void observe(Context next,unsigned epoch,uint16_t value,uint64_t now){
        value=next==Context::None?0:uint16_t(value&AllIcons);
        if(next!=context || epoch!=profile || value!=mask)started=now;
        context=next;profile=epoch;mask=value;
    }
    float scale(uint64_t now) const {
        const uint64_t age=now>=started?now-started:0;
        if(age>=500000)return 17/32.f;
        const float t=(age%250000)/250000.f,u=1-t;
        const float cubic=t<.5f?4*t*t*t:1-4*u*u*u;
        return (17/32.f)*(age<250000?1+.5f*cubic:1.5f-.5f*cubic);
    }
};
template<class Emit>inline void forEach(const State& value,uint64_t now,int left,Emit emit){
    if(value.context==Context::None)return;
    constexpr float raster=17/32.f;const float scale=value.scale(now);unsigned cell=0;
    for(auto i:Order)if(value.mask&(1u<<i)){
        emit(i,left+(4+36*(cell%5)+16)*raster-16*scale,(4+36*(cell/5)+16)*raster-16*scale,scale);++cell;
    }
}
} // namespace ModifierHud061
#endif
