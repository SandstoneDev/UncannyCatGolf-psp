#pragma once
// Include inside Main's namespace after Patch/GU helpers. Baselinegift_ui.hpp
// remains unchanged; the integration replaces only its blast storage/load/draw.
#include "editor_legacy_gift_blast_data.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <zlib.h>
namespace EditorLegacyGiftBlast {
inline constexpr const char* BankPath=EditorLegacyGiftBlastData::BankPath;
inline constexpr unsigned FrameBytes=262144,WorkspaceBytes=65536,StaticPixelBytes=327680;
alignas(64) inline uint8_t pixels[FrameBytes],workspace[WorkspaceBytes];
inline FILE* file=nullptr;
inline int cachedFrame=-1;
inline const char* lastError=nullptr;
inline constexpr unsigned PixelLease=1,WorkspaceLease=2;
inline unsigned builderLeases=0;

inline bool builderOwnsBuffers(){return builderLeases!=0;}

// Lease callbacks run on Main. Acquisition changes ownership only: an open
// legacy list can still read pixels until the Builder renderer fences it.
inline void* borrowBuilderBuffer(size_t bytes,void*){
 const unsigned lease=bytes==FrameBytes?PixelLease:bytes==WorkspaceBytes?WorkspaceLease:0;
 if(!lease||(builderLeases&lease))return nullptr;
 builderLeases|=lease;cachedFrame=-1;
 return lease==PixelLease?pixels:workspace;
}
inline void returnBuilderBuffer(void* buffer,void*){
 if(buffer==pixels)builderLeases&=~PixelLease;
 else if(buffer==workspace)builderLeases&=~WorkspaceLease;
 else return;
 cachedFrame=-1;
}
inline unsigned decodes=0,cacheHits=0,arenaPeak=0;
struct Arena{unsigned used=0;};
inline voidpf allocate(voidpf context,uInt n,uInt size){
 auto& a=*static_cast<Arena*>(context);if(!n||!size||n>WorkspaceBytes/size)return nullptr;
 const unsigned bytes=n*size,at=(a.used+63)&~63u;if(at>WorkspaceBytes||bytes>WorkspaceBytes-at)return nullptr;
 auto*p=workspace+at;std::memset(p,0,bytes);a.used=at+bytes;return p;
}
inline void release(voidpf,voidpf){}
inline bool fence(){
 if(sceGuFinish()<0){lastError="Cannot finish legacy gift GE list";return false;}
 if(sceGuSync(GU_SYNC_FINISH,GU_SYNC_WHAT_DONE)<0){lastError="Cannot fence legacy gift pixels";return false;}
 if(sceGuStart(GU_DIRECT,displayList)<0){lastError="Cannot resume legacy gift GE list";return false;}
 return true;
}
// false is legal only after the caller has already submitted/fenced GE reads.
inline bool close(bool fenceReaders=true){
 if(builderOwnsBuffers())return false;
 if(cachedFrame>=0&&fenceReaders&&!fence())return false;
 cachedFrame=-1;if(file)std::fclose(file);file=nullptr;return true;
}
inline bool load(const char* path){
 if(!close()){return false;}lastError=nullptr;
 file=path?std::fopen(path,"rb"):nullptr;if(!file){lastError="Cannot open lossless legacy gift blast bank";return false;}
 uint32_t header[4]{};bool good=std::fread(header,sizeof header,1,file)==1&&header[0]==0x42474c55&&header[1]==1&&header[2]==EditorLegacyGiftBlastData::Frames&&header[3]==EditorLegacyGiftBlastData::BankBytes;
 for(const auto& f:EditorLegacyGiftBlastData::Data){uint32_t r[4]{};if(std::fread(r,sizeof r,1,file)!=1||r[0]!=f.offset||r[1]!=f.compressed||r[2]!=f.raw||r[3]!=f.crc)good=false;}
 if(!good||std::fseek(file,0,SEEK_END)||std::ftell(file)!=long(EditorLegacyGiftBlastData::BankBytes)){std::fclose(file);file=nullptr;lastError="Invalid legacy gift blast bank";return false;}
 return true;
}
inline bool prepare(unsigned frame){
 lastError=nullptr;
 if(builderOwnsBuffers()){lastError="Builder gift owns legacy buffers";return false;}
 if(!file||frame>=EditorLegacyGiftBlastData::Frames){lastError="Invalid legacy gift frame request";return false;}
 if(cachedFrame==int(frame)){++cacheHits;return true;}
 // No CPU write may precede this submit+sync: the old frame can still be
 // referenced in an OPEN, unsent Main list, including multiple same-framehits.
 if(!fence())return false;
 cachedFrame=-1;const auto& record=EditorLegacyGiftBlastData::Data[frame];
 if(std::fseek(file,long(record.offset),SEEK_SET)){lastError="Cannot seek legacy gift frame";return false;}
 Arena arena;z_stream z{};z.zalloc=allocate;z.zfree=release;z.opaque=&arena;
 if(inflateInit(&z)!=Z_OK){lastError="Cannot initialize legacy gift decoder";return false;}
 uint8_t input[4096];unsigned left=record.compressed;z.next_out=pixels;z.avail_out=FrameBytes;bool good=false;
 for(;;){
  if(!z.avail_in&&left){const unsigned n=std::min<unsigned>(sizeof input,left);if(std::fread(input,1,n,file)!=n)break;left-=n;z.next_in=input;z.avail_in=n;}
  const int result=inflate(&z,Z_NO_FLUSH);
  if(result==Z_STREAM_END){good=!left&&!z.avail_in&&z.total_out==FrameBytes&&crc32(crc32(0,nullptr,0),pixels,FrameBytes)==record.crc;break;}
  if(result!=Z_OK||!z.avail_out||(!z.avail_in&&!left))break;
 }
 inflateEnd(&z);arenaPeak=std::max(arenaPeak,arena.used);
 if(!good){lastError="Cannot decode exact legacy gift frame";return false;}
 sceKernelDcacheWritebackRange(pixels,FrameBytes);sceGuTexFlush();cachedFrame=int(frame);++decodes;return true;
}
inline bool draw(unsigned frame,float originX,float originY,float scale=1.f,uint32_t tint=0xffffffffu){
 if(!prepare(frame))return false;
 const auto& f=EditorLegacyGiftBlastData::Data[frame];setTexture(GU_PSM_8888,256,256,pixels);
 const Patch p{1,1,int16_t(f.w),int16_t(f.h),int16_t(f.x),int16_t(f.y)};
 patch(p,originX+f.x*scale,originY+f.y*scale,scale,tint);return true;
}
inline const char* error(){return lastError?lastError:"";}
}
