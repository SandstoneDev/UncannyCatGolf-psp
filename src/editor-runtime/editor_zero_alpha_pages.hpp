#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
namespace EditorZeroAlphaPages {
inline bool finite(float value){uint32_t bits;std::memcpy(&bits,&value,4);return (bits&0x7f800000u)!=0x7f800000u;}
// Facts learned only from a validated immutable builtin T8 page's complete
// 256-entry palette. No pointers or custom-image facts survive the observation.
// Clear at every asset/cache lifecycle boundary before IDs can name new data.
template<unsigned Pages>class Set{
 static_assert(Pages>0,"bounded page IDs");
 std::array<uint32_t,(Pages+31)/32> bits{};
public:
 static constexpr size_t fixedBytes(){return sizeof(Set);}
 void clear(){bits.fill(0);}
 bool contains(unsigned page)const{return page<Pages&&(bits[page/32]&(1u<<(page%32)))!=0;}
 bool canSkip(unsigned page,float sourceAlpha,float ambientAlpha)const{return contains(page)&&finite(sourceAlpha)&&finite(ambientAlpha);}
 bool observe(unsigned page,const uint32_t*palette,unsigned colors,bool validatedBuiltinT8){
  if(page>=Pages||!validatedBuiltinT8||!palette||!colors||colors>256)return false;
  if(contains(page))return true;
  for(unsigned i=0;i<256;++i)if(palette[i]>>24)return false;
  bits[page/32]|=1u<<(page%32);return true;
 }
};
}
