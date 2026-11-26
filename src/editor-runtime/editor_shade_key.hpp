#pragma once
#include "editor_sample_cache.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
namespace EditorShadeKey {
inline constexpr std::size_t MaxKeyBytes=4096,MaxSelected=16,MaxLights=256;
inline uint32_t bits(const float& value){static_assert(sizeof(float)==4,"IEEE32 shader identity required");uint32_t word;std::memcpy(&word,&value,4);return word;}
struct Float4{float r=1,g=1,b=1,a=1;};
struct Shader{float cameraX=0,cameraY=0,centerX=0,centerY=0,zoom=1,step=1,originX=0,originY=0;};
struct Input{
 EditorSampleCache::Key source{};
 Shader shader{};
 int32_t left=0,top=0;uint32_t width=0,height=0;
 Float4 nodeTint{};
 const unsigned* selected=nullptr;std::size_t selectedCount=0;
};
enum class Status{Ok,Invalid,Oversize};
// The length excludes the terminator. A failed encode publishes only an empty
// key; it never truncates. The full UV plan is compared separately by the cache.
inline Status encode(const Input& in,char* output,std::size_t capacity,std::size_t& length){
 length=0;if(output&&capacity)output[0]=0;
 if(!output||!capacity||in.selectedCount>MaxSelected||(in.selectedCount&&!in.selected)||(in.source.kind!=EditorSampleCache::Kind::Bank&&in.source.kind!=EditorSampleCache::Kind::Custom))return Status::Invalid;
 const std::size_t nameBytes=in.source.kind==EditorSampleCache::Kind::Custom?in.source.nameBytes:0;
 if(in.source.kind==EditorSampleCache::Kind::Custom&&(!in.source.name||!nameBytes))return Status::Invalid;
 constexpr std::size_t FixedChars=25*8;
 if(nameBytes>(MaxKeyBytes-1-FixedChars)/2)return Status::Oversize;
 const std::size_t needed=FixedChars+nameBytes*2+in.selectedCount*8;
 if(needed>=MaxKeyBytes||needed>=capacity)return Status::Oversize;
 char* cursor=output;constexpr char hex[]="0123456789abcdef";
 const auto word=[&](uint32_t value){for(unsigned shift=32;shift;){shift-=4;*cursor++=hex[(value>>shift)&15];}};
 word(0x4c495430);word(uint32_t(in.source.kind));word(in.source.pageId);word(in.source.x);word(in.source.y);word(in.source.w);word(in.source.h);word(uint32_t(nameBytes));
 for(std::size_t i=0;i<nameBytes;++i){const unsigned byte=static_cast<unsigned char>(in.source.name[i]);*cursor++=hex[byte>>4];*cursor++=hex[byte&15];}
 word(bits(in.shader.cameraX));word(bits(in.shader.cameraY));word(bits(in.shader.centerX));word(bits(in.shader.centerY));word(bits(in.shader.zoom));word(bits(in.shader.step));word(bits(in.shader.originX));word(bits(in.shader.originY));
 word(uint32_t(in.left));word(uint32_t(in.top));word(in.width);word(in.height);
 word(bits(in.nodeTint.r));word(bits(in.nodeTint.g));word(bits(in.nodeTint.b));word(bits(in.nodeTint.a));word(uint32_t(in.selectedCount));
 for(std::size_t i=0;i<in.selectedCount;++i)word(uint32_t(in.selected[i]));
 *cursor=0;length=std::size_t(cursor-output);return Status::Ok;
}
enum class EpochStatus{Unchanged,Changed,Disabled};
class Epoch {
 using PointWords=std::array<uint32_t,12>;
 std::array<PointWords,MaxLights> snapshot{};
 std::array<uint32_t,4> ambient{};
 uint64_t maskGeneration=0;
 std::size_t pointCount=0;
 bool valid=false;
 template<class P>static PointWords words(const P& p){return {bits(p.x),bits(p.y),bits(p.xx),bits(p.xy),bits(p.yx),bits(p.yy),bits(p.r),bits(p.g),bits(p.b),bits(p.a),bits(p.extentX),bits(p.extentY)};}
public:
 // Disabled invalidates the old snapshot. Caller must bypass AND clear its lit
 // cache; later return to a supported count will produce Changed, never a hit.
 void invalidate(){valid=false;}
 template<class Ambient,class Point>EpochStatus update(const Ambient& color,const Point* points,std::size_t count,uint64_t lightmaskGeneration){
  if(count>MaxLights||(count&&!points)){invalidate();return EpochStatus::Disabled;}
  const std::array<uint32_t,4> current{bits(color.r),bits(color.g),bits(color.b),bits(color.a)};
  bool changed=!valid||count!=pointCount||maskGeneration!=lightmaskGeneration||current!=ambient;
  for(std::size_t i=0;i<count;++i){const PointWords point=words(points[i]);if(point!=snapshot[i])changed=true;snapshot[i]=point;}
  ambient=current;pointCount=count;maskGeneration=lightmaskGeneration;valid=true;
  return changed?EpochStatus::Changed:EpochStatus::Unchanged;
 }
 bool enabled()const{return valid;}
};
inline constexpr std::size_t EpochBytes=sizeof(Epoch),DynamicBytes=0;
static_assert(EpochBytes<=20*1024,"Lighting epoch metadata must stay bounded");
}
