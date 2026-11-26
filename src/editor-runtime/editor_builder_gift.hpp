#pragma once
#include "editor_builder_gift_data.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iterator>
#include <zlib.h>
namespace EditorBuilderGift {
inline constexpr const char* BankPath="data/world/editor-gift.bin";
inline constexpr const char* BuilderPath="data/world/editor-aux/player/costume_builder.png";
inline constexpr const char* BoomPath="data/world/editor-aux/minigames/bartbash/boom.png";
inline constexpr unsigned WorkspaceBytes=65536,OutputBytes=66560;
struct Rect{float x,y,w,h;};
struct Params{
 float x=227,y=128,scale=.25f,darkAlpha=0;
 unsigned spinFrame=0;
 int explodeMs=-1;
 bool shown=false,ru=false;
};
enum class Kind{Solid,Bank,External};
struct Command{
 Kind kind;unsigned record;const char* path;
 Rect source,dest,clip;bool clipped;uint32_t tint;
};
// Coordinates are authored 768x512. The caller clips to the physical PSP viewport
// and applies ViewX=36,scale=17/32 only once. No image pixels are downsampled here.
template<class Emit> void visit(const Params& v,Emit emit){
 const unsigned alpha=unsigned(std::max(0.f,std::min(1.f,v.darkAlpha))*230.f+.5f);
 if(alpha)emit(Command{Kind::Solid,0,nullptr,{0,0,0,0},{256,0,512,512},{0,0,0,0},false,alpha<<24});
 if(!v.shown||v.scale<=0)return;
 const Data::Pose idle{0,1,1,0,1};
 const auto& pose=v.explodeMs<0?idle:Data::Poses[std::min(1000u,unsigned(v.explodeMs))];
 const auto transformed=[&](Rect r){return Rect{v.x+r.x*v.scale,v.y+r.y*v.scale,r.w*v.scale,r.h*v.scale};};
 if(pose.flags&1){const float s=pose.presentScale;emit(Command{Kind::Bank,v.spinFrame%16,nullptr,{0,0,256,256},transformed({pose.presentX-128*s,-128*s,256*s,256*s}),{},false,0xffffffff});}
 if((pose.flags&2)&&pose.detailsScale>0){
  const auto& box=Data::Boxes[unsigned(v.ru)];const float s=pose.detailsScale;
  const auto detail=[&](Rect r){return transformed({r.x*s,r.y*s,r.w*s,r.h*s});};
  const Rect clip=detail({box.x,box.y,box.w,box.h});
  for(unsigned label=0;label<3;++label){
   const auto& l=Data::Labels[unsigned(v.ru)][label];
   for(unsigned j=0;j<l.count;++j){const auto& p=Data::Parts[l.first+j];emit(Command{Kind::Bank,p.record,nullptr,{0,0,p.w,p.h},detail({box.x+l.x+p.x,box.y+l.y+p.y,p.w,p.h}),clip,true,0xffffffff});}
   if(label==0)emit(Command{Kind::External,0,BuilderPath,{0,0,768,768},detail({box.x+box.iconX,box.y+box.iconY,box.iconW,box.iconH}),clip,true,0xffffffff});
  }
 }
 if(pose.flags&4)emit(Command{Kind::External,0,BoomPath,{float(std::min(pose.boomFrame,16u)*71),0,71,100},transformed({-142,-200,284,400}),{},false,0xffffffff});
}
class Reader{
 FILE* file=nullptr;
 struct Arena{uint8_t* memory;std::size_t used=0;};
 static voidpf allocate(voidpf opaque,uInt count,uInt size){
  auto& a=*static_cast<Arena*>(opaque);if(!count||!size||count>WorkspaceBytes/size)return nullptr;
  const std::size_t bytes=std::size_t(count)*size,at=(a.used+63)&~std::size_t(63);
  if(at>WorkspaceBytes||bytes>WorkspaceBytes-at)return nullptr;
  auto* p=a.memory+at;std::memset(p,0,bytes);a.used=at+bytes;return p;
 }
 static void release(voidpf,voidpf){}
public:
 int error=0;unsigned workspacePeak=0;
 Reader()=default;Reader(const Reader&)=delete;Reader& operator=(const Reader&)=delete;
 ~Reader(){close();}
 void close(){if(file)std::fclose(file);file=nullptr;}
 bool open(const char* path){
  close();error=-1;file=std::fopen(path,"rb");if(!file)return false;
  uint32_t h[4]{};if(std::fread(h,sizeof h,1,file)!=1||h[0]!=0x47424355||h[1]!=1||h[2]!=std::size(Data::Records)||h[3]!=Data::BankBytes||std::fseek(file,0,SEEK_END)||std::ftell(file)!=long(Data::BankBytes)){close();return false;}
  error=0;return true;
 }
 // Output layout: swizzled T8 indices pw*ph bytes, then 256 exact RGBA8888
 // CLUT entries. Callers must fence GE before overwriting an in-use output page.
 bool decode(unsigned record,void* output,std::size_t capacity,void* workspace,std::size_t workspaceSize){
  error=-2;if(!file||record>=std::size(Data::Records)||!output||!workspace)return false;
  const auto& r=Data::Records[record];const uintptr_t raw=reinterpret_cast<uintptr_t>(workspace),aligned=(raw+63)&~uintptr_t(63);
  if(raw>UINTPTR_MAX-63||workspaceSize<aligned-raw||workspaceSize-(aligned-raw)<WorkspaceBytes||capacity<r.raw)return false;
  if(std::fseek(file,r.offset,SEEK_SET)){error=-3;return false;}
  Arena arena{reinterpret_cast<uint8_t*>(aligned)};z_stream z{};z.zalloc=allocate;z.zfree=release;z.opaque=&arena;
  int result=inflateInit(&z);if(result!=Z_OK){error=result;return false;}
  uint8_t input[4096];unsigned left=r.bytes;z.next_out=static_cast<Bytef*>(output);z.avail_out=r.raw;bool good=false;
  for(;;){
   if(!z.avail_in&&left){unsigned n=std::min<unsigned>(sizeof input,left);if(std::fread(input,1,n,file)!=n)break;left-=n;z.next_in=input;z.avail_in=n;}
   result=inflate(&z,Z_NO_FLUSH);
   if(result==Z_STREAM_END){good=!left&&!z.avail_in&&z.total_out==r.raw&&crc32(crc32(0,nullptr,0),static_cast<Bytef*>(output),r.raw)==r.crc;break;}
   if(result!=Z_OK||!z.avail_out||(!z.avail_in&&!left))break;
  }
  inflateEnd(&z);workspacePeak=std::max(workspacePeak,unsigned(arena.used));error=good?0:-4;return good;
 }
};
}
