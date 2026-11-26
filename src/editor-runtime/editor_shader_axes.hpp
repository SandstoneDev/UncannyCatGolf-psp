#pragma once
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#ifndef __PSP__
#include <cfenv>
#ifdef __SSE__
#include <xmmintrin.h>
#endif
#endif
namespace EditorShaderAxes {
inline constexpr unsigned Width=480,Height=272;
// Only value-affecting controls belong in the key, not sticky exception flags.
inline uint32_t nativeMode(){
#ifdef __PSP__
 uint32_t value;__asm__ volatile("cfc1 %0, $31":"=r"(value));return value&0x01000f83u;
#else
 uint32_t value=std::fegetround()==FE_TONEAREST?0u:1u;
#ifdef __SSE__
 const auto control=_mm_getcsr();value|=control&0xe040u;
 if((control&0x1f80u)!=0x1f80u)value|=0x100u;
#else
 value|=0x100u; // Unknown host floating controls stay on the original path.
#endif
 return value;
#endif
}
struct Key {
 double cameraX,cameraY,centerX,centerY;
 float step,scale;int originX;uint32_t mode;
};
static_assert(sizeof(Key)==48&&offsetof(Key,step)==32&&offsetof(Key,mode)==44,"complete key without padding");
inline float xExpression(const Key&key,int left,unsigned x){return float(key.cameraX)+(left+x+.5f-key.originX-float(key.centerX)*key.scale)*key.step;}
inline float yExpression(const Key&key,int top,unsigned y){return float(key.cameraY)+(top+y+.5f-float(key.centerY)*key.scale)*key.step;}
struct Axes {std::array<float,Width> x{};std::array<float,Height> y{};};
class Cache {
 Axes axes{};Key key{};bool ready=false;
 static bool supported(const Key&value){
  // Flush/DAZ, non-nearest rounding and enabled traps retain the original path.
  return value.mode==0&&std::isfinite(value.cameraX)&&std::isfinite(value.cameraY)&&
   std::isfinite(value.centerX)&&std::isfinite(value.centerY)&&std::isfinite(value.step)&&std::isfinite(value.scale)&&
   std::isfinite(float(value.cameraX))&&std::isfinite(float(value.cameraY))&&
   std::isfinite(float(value.centerX))&&std::isfinite(float(value.centerY));
 }
public:
 static constexpr size_t fixedBytes(){return sizeof(Cache);}
 void clear(){ready=false;}
 // Evaluators are pure functions of the complete key. Returned storage is local
 // to this View and remains valid until a different key is prepared or cleared.
 template<class X,class Y>const Axes*prepare(const Key&value,const X&x,const Y&y){
  if(!supported(value)){ready=false;return nullptr;}
  if(ready&&!std::memcmp(&key,&value,sizeof key))return &axes;
  for(unsigned i=0;i<Width;++i)axes.x[i]=x(i);
  for(unsigned i=0;i<Height;++i)axes.y[i]=y(i);
  key=value;ready=true;return &axes;
 }
};
static_assert(sizeof(Cache)<=3072,"at most 3 KiB per View");
inline const Axes*bind(Cache&cache,const Key&key,int left,int top,unsigned w,unsigned h){
 // Preserve the original unsigned left+x/top+y expression outside the viewport.
 if(left<0||top<0||uint64_t(unsigned(left))+w>Width||uint64_t(unsigned(top))+h>Height)return nullptr;
 return cache.prepare(key,[&](unsigned x){return xExpression(key,0,x);},[&](unsigned y){return yExpression(key,0,y);});
}
}
