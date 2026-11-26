#pragma once
#include "editor_builder_gift.hpp"
#include "editor_builder_gift_source.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <new>
namespace EditorBuilderGiftRaster {
inline constexpr unsigned OutputBytes=262144,WorkspaceBytes=65536,VisitedBytes=8192;
inline constexpr float Scale=17.f/32.f,ViewX=36.f;
struct PhysicalClip{float left=36,top=0,right=444,bottom=272;};
struct Texture{int x=0,y=0;unsigned width=0,height=0,potWidth=0,potHeight=0,bytes=0;const char* error=nullptr;};
enum class Result{Empty,Ready,Error};
inline unsigned swizzledIndex(unsigned index,unsigned width,unsigned rowShift){
 const unsigned x=index&(width-1),y=index>>rowShift;
 return (((y>>3)*(width>>2)+(x>>2))<<5)+((y&7)<<2)+(x&3);
}
// Scatter each old linear RGBA word to its swizzled word index, cycle by cycle.
// The caller supplies only a one-bit-per-word visited map, never another image.
inline bool swizzleInPlace(void* rgba,unsigned width,unsigned height,void* workspace,std::size_t workspaceBytes){
 if(!rgba||!workspace||width<16||height<8||width>256||height>256||(width&(width-1))||(height&(height-1)))return false;
 const unsigned words=width*height,bytes=(words+7)/8;if(workspaceBytes<bytes)return false;
 unsigned rowShift=0;for(unsigned n=width;n>1;n>>=1)++rowShift;
 auto* visited=static_cast<uint8_t*>(workspace);std::memset(visited,0,bytes);auto* pixels=static_cast<uint8_t*>(rgba);
 for(unsigned start=0;start<words;++start){
  if(visited[start>>3]&(1u<<(start&7)))continue;
  unsigned current=start;uint32_t carried;std::memcpy(&carried,pixels+current*4,4);
  do{const unsigned next=swizzledIndex(current,width,rowShift);uint32_t replaced;std::memcpy(&replaced,pixels+next*4,4);std::memcpy(pixels+next*4,&carried,4);carried=replaced;visited[current>>3]|=uint8_t(1u<<(current&7));current=next;}while(current!=start);
 }
 return true;
}
inline bool finite(const EditorBuilderGift::Rect&r){return std::isfinite(r.x)&&std::isfinite(r.y)&&std::isfinite(r.w)&&std::isfinite(r.h);}
inline bool overlaps(const void*a,std::size_t na,const void*b,std::size_t nb){
 const uintptr_t aa=reinterpret_cast<uintptr_t>(a),bb=reinterpret_cast<uintptr_t>(b);
 return na>UINTPTR_MAX-aa||nb>UINTPTR_MAX-bb||(aa<bb+nb&&bb<aa+na);
}
inline Result rasterize(const char* path,const EditorBuilderGift::Command& command,PhysicalClip clip,void* output,std::size_t outputBytes,void* workspace,std::size_t workspaceBytes,Texture& texture){
 texture={};
 auto fail=[&](const char* error){texture.error=error;return Result::Error;};
 if(command.kind!=EditorBuilderGift::Kind::External||!finite(command.source)||!finite(command.dest)||(command.clipped&&!finite(command.clip))||!std::isfinite(clip.left)||!std::isfinite(clip.top)||!std::isfinite(clip.right)||!std::isfinite(clip.bottom))return fail("Invalid gift external geometry");
 if(command.dest.w<=0||command.dest.h<=0)return Result::Empty;
 if(command.source.w<=0||command.source.h<=0)return fail("Invalid gift source rectangle");
 const double dx=ViewX+double(command.dest.x)*Scale,dy=double(command.dest.y)*Scale,dw=double(command.dest.w)*Scale,dh=double(command.dest.h)*Scale;
 double left=std::max({dx,double(clip.left),36.0}),top=std::max({dy,double(clip.top),0.0}),right=std::min({dx+dw,double(clip.right),444.0}),bottom=std::min({dy+dh,double(clip.bottom),272.0});
 if(command.clipped){
  if(command.clip.w<=0||command.clip.h<=0)return Result::Empty;
  const double cx=ViewX+double(command.clip.x)*Scale,cy=double(command.clip.y)*Scale;
  left=std::max(left,cx);top=std::max(top,cy);right=std::min(right,cx+double(command.clip.w)*Scale);bottom=std::min(bottom,cy+double(command.clip.h)*Scale);
 }
 if(!(left<right&&top<bottom))return Result::Empty;
 const int x=int(std::ceil(left-.5)),y=int(std::ceil(top-.5)),endX=int(std::ceil(right-.5)),endY=int(std::ceil(bottom-.5));
 if(x>=endX||y>=endY)return Result::Empty;
 const unsigned width=unsigned(endX-x),height=unsigned(endY-y);if(width>256||height>256)return fail("Gift physical raster needs clipped chunks");
 unsigned pw=16,ph=8;while(pw<width)pw*=2;while(ph<height)ph*=2;const unsigned bytes=pw*ph*4;
 if(!path||!output||!workspace||outputBytes<bytes||reinterpret_cast<uintptr_t>(output)%alignof(uint32_t)||workspaceBytes<WorkspaceBytes||overlaps(output,bytes,workspace,workspaceBytes))return fail("Invalid gift raster buffers");
 const uintptr_t raw=reinterpret_cast<uintptr_t>(workspace),aligned=(raw+alignof(EditorBuilderGiftSource::Workspace)-1)&~uintptr_t(alignof(EditorBuilderGiftSource::Workspace)-1);
 if(raw>UINTPTR_MAX-(alignof(EditorBuilderGiftSource::Workspace)-1)||aligned-raw>workspaceBytes||workspaceBytes-(aligned-raw)<sizeof(EditorBuilderGiftSource::Workspace))return fail("Invalid gift sampling workspace");
 unsigned asset=unsigned(std::size(EditorBuilderGiftSourceData::Images));
 if(command.path)for(unsigned i=0;i<std::size(EditorBuilderGiftSourceData::Images);++i)if(!std::strcmp(command.path,EditorBuilderGiftSourceData::Images[i].path)){asset=i;break;}
 if(asset>=std::size(EditorBuilderGiftSourceData::Images))return fail("Unknown original gift image");
 const auto& info=EditorBuilderGiftSourceData::Images[asset];
 auto* samples=new(reinterpret_cast<void*>(aligned)) EditorBuilderGiftSource::Workspace;
 EditorBuilderGiftSource::Reader reader;if(!reader.open(path,asset,samples->palette))return fail("Cannot open original gift source row bank");
 for(unsigned i=0;i<width;++i){const double source=double(command.source.x)+(double(x)+i+.5-dx)/dw*command.source.w;samples->xs[i]=unsigned(std::clamp(std::floor(source),0.0,double(info.width-1)));}
 for(unsigned i=0;i<height;++i){const double source=double(command.source.y)+(double(y)+i+.5-dy)/dh*command.source.h;samples->ys[i]=unsigned(std::clamp(std::floor(source),0.0,double(info.height-1)));}
 std::memset(output,0,bytes);unsigned previous=UINT32_MAX;
 auto* arena=reinterpret_cast<uint8_t*>(samples)+sizeof(*samples);const std::size_t arenaBytes=workspaceBytes-std::size_t(arena-static_cast<uint8_t*>(workspace));
 for(unsigned row=0;row<height;++row){
  const unsigned sourceY=samples->ys[row];if(sourceY!=previous){if(!reader.readRow(sourceY,*samples,arena,arenaBytes))return fail("Cannot decode original gift source row");previous=sourceY;}
  auto* pixels=static_cast<uint32_t*>(output)+row*pw;for(unsigned col=0;col<width;++col)pixels[col]=samples->palette[samples->row[samples->xs[col]]];
 }
 reader.close();samples->~Workspace();
 if(!swizzleInPlace(output,pw,ph,workspace,workspaceBytes))return fail("Cannot swizzle gift raster");
 texture={x,y,width,height,pw,ph,bytes,nullptr};return Result::Ready;
}
}
