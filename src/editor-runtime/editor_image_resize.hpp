/**************************************************************************/
/*  image.cpp                                                             */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#pragma once
#include <memory>
#include <vector>
namespace EditorImages {
struct ResizeAxis { unsigned a,b,f; };
inline ResizeAxis resizeAxis(unsigned i,unsigned source,unsigned destination){
 const unsigned fp=unsigned(uint64_t(2*i+1)*source*128/destination);
 return {fp>=128?(fp-128)>>8:0,std::min(source-1,(fp+128)>>8),(fp&255)>=128?(fp&255)-128:(fp&255)+128};
}
// Same RGBA8 Q8 interpolation as Godot 4.4 Image.resize(BILINEAR). Store
// only the distinct source texels needed by the two-tap filter. Adam7 is
// merged by the common decoder; no full-resolution custom image allocation.
inline bool resizedBilinear(const char*path,uint32_t*out,unsigned width,unsigned height,unsigned stride){
 if(!path||!out||!width||!height||width>512||height>512||stride<width)return false;
 Info info;if(!inspect(path,info))return false;
 Samples samples;samples.fixedCoordinates=true;
 ResizeAxis ax[512],ay[512];
 auto axis=[&](unsigned count,unsigned source,ResizeAxis*map,unsigned*coordinates,unsigned&size){
  for(unsigned i=0;i<count;++i){const auto p=resizeAxis(i,source,count);coordinates[size++]=p.a;coordinates[size++]=p.b;}
  std::sort(coordinates,coordinates+size);size=unsigned(std::unique(coordinates,coordinates+size)-coordinates);
  for(unsigned i=0;i<count;++i){auto p=resizeAxis(i,source,count);map[i]={unsigned(std::lower_bound(coordinates,coordinates+size,p.a)-coordinates),unsigned(std::lower_bound(coordinates,coordinates+size,p.b)-coordinates),p.f};}
 };
 axis(width,info.width,ax,samples.xs,samples.width);axis(height,info.height,ay,samples.ys,samples.height);
 std::unique_ptr<uint32_t[]> pixels(new(std::nothrow) uint32_t[size_t(samples.width)*samples.height]());
 if(!pixels)return false;
 const Window window{0,0,samples.width,samples.height};
 if(!load(path,info,&window,pixels.get(),samples.width,&samples))return false;
 for(unsigned y=0;y<height;++y)for(unsigned x=0;x<width;++x){
  const auto&a=ax[x];const auto&b=ay[y];uint32_t result=0;
  for(unsigned channel=0;channel<32;channel+=8){
   const uint32_t p00=((pixels[b.a*samples.width+a.a]>>channel)&255)<<8,p10=((pixels[b.a*samples.width+a.b]>>channel)&255)<<8;
   const uint32_t p01=((pixels[b.b*samples.width+a.a]>>channel)&255)<<8,p11=((pixels[b.b*samples.width+a.b]>>channel)&255)<<8;
   const uint32_t top=p00+(((p10-p00)*a.f)>>8),bottom=p01+(((p11-p01)*a.f)>>8);
   const uint32_t v=(top+(((bottom-top)*b.f)>>8))>>8;result|=(v&255)<<channel;
  }out[y*stride+x]=result;
 }return true;
}
} // namespace EditorImages
