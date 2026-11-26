#pragma once
#include <algorithm>
#include <cstdint>
#include <cstring>
namespace EditorRaster {
struct Texture {unsigned width=0,height=0,bytes=0;};
inline Texture dimensions(unsigned w,unsigned h){if(!w||!h||w>256||h>256)return {};unsigned tw=16,th=8;while(tw<w)tw*=2;while(th<h)th*=2;return {tw,th,tw*th*4};}
inline void clear(uint32_t*surface,unsigned stride,unsigned w,unsigned h){for(unsigned y=0;y<h;++y)std::fill_n(surface+y*stride,w,0u);}
// Exact same visible RGBA texels, compact PSP 16-byte x 8-row swizzle blocks.
// Source assets retain their original size; this is the final framebuffer raster.
inline Texture pack(const uint32_t*source,unsigned stride,uint8_t*out,unsigned w,unsigned h){
 const auto texture=dimensions(w,h);if(!source||!out||!texture.bytes||stride<w)return {};
 unsigned at=0;
 for(unsigned by=0;by<texture.height;by+=8)for(unsigned bx=0;bx<texture.width;bx+=4)for(unsigned row=0;row<8;++row){
  unsigned count=by+row<h&&bx<w?std::min(4u,w-bx):0;
  if(count)std::memcpy(out+at,source+(by+row)*stride+bx,count*4);
  if(count<4)std::memset(out+at+count*4,0,(4-count)*4);
  at+=16;
 }return texture;
}
}
