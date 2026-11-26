#pragma once
#include "editor_nuke_data.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <zlib.h>
namespace EditorNukeArt {
class Reader {
 std::FILE* file=nullptr;
 struct Arena{uint8_t* memory;std::size_t used=0;};
 static voidpf allocate(voidpf opaque,uInt count,uInt size){
  auto&a=*static_cast<Arena*>(opaque);if(!count||!size||count>WorkspaceBytes/size)return nullptr;
  const std::size_t bytes=std::size_t(count)*size,at=(a.used+63)&~std::size_t(63);
  if(at>WorkspaceBytes||bytes>WorkspaceBytes-at)return nullptr;
  auto*p=a.memory+at;std::memset(p,0,bytes);a.used=at+bytes;return p;
 }
 static void release(voidpf,voidpf){}
public:
 int error=0;unsigned workspacePeak=0;
 Reader()=default;Reader(const Reader&)=delete;Reader&operator=(const Reader&)=delete;
 ~Reader(){close();}
 void close(){if(file)std::fclose(file);file=nullptr;}
 bool open(const char* path){
  close();error=-1;if(!path)return false;file=std::fopen(path,"rb");if(!file)return false;
  uint32_t header[4]{};
  if(std::fread(header,sizeof header,1,file)!=1||header[0]!=0x4e494355||header[1]!=1||header[2]!=FrameCount||header[3]!=BankBytes||std::fseek(file,0,SEEK_END)||std::ftell(file)!=long(BankBytes)){close();return false;}
  error=0;return true;
 }
 // Existing six-frame LRU and GE fences belong to the caller. No heap or
 // full RGBA frame scratch; output is exact swizzled T8+256 RGBA CLUT words.
 bool decode(unsigned frame,void* output,std::size_t capacity,void* workspace,std::size_t workspaceSize){
  error=-2;if(!file||frame>=FrameCount||!output||!workspace||capacity<FrameBytes)return false;
  const uintptr_t raw=reinterpret_cast<uintptr_t>(workspace),target=reinterpret_cast<uintptr_t>(output);
  if(raw>UINTPTR_MAX-63||raw>UINTPTR_MAX-workspaceSize||target>UINTPTR_MAX-FrameBytes)return false;
  const uintptr_t aligned=(raw+63)&~uintptr_t(63);
  if(workspaceSize<aligned-raw||workspaceSize-(aligned-raw)<WorkspaceBytes||
     (target<raw+workspaceSize&&raw<target+FrameBytes))return false;
  const auto&r=Records[frame];if(r.raw!=FrameBytes||!r.bytes||r.offset>BankBytes||r.bytes>BankBytes-r.offset||!r.colors||r.colors>256)return false;
  if(std::fseek(file,r.offset,SEEK_SET)){error=-3;return false;}
  Arena arena{reinterpret_cast<uint8_t*>(aligned)};z_stream z{};z.zalloc=allocate;z.zfree=release;z.opaque=&arena;
  int result=inflateInit(&z);if(result!=Z_OK){error=result;return false;}
  uint8_t input[4096];unsigned left=r.bytes;z.next_out=static_cast<Bytef*>(output);z.avail_out=r.raw;bool good=false;
  for(;;){
   if(!z.avail_in&&left){const unsigned bytes=std::min<unsigned>(sizeof input,left);if(std::fread(input,1,bytes,file)!=bytes)break;left-=bytes;z.next_in=input;z.avail_in=bytes;}
   const auto beforeIn=z.total_in,beforeOut=z.total_out;result=inflate(&z,Z_NO_FLUSH);
   if(result==Z_STREAM_END){good=!left&&!z.avail_in&&z.total_out==r.raw&&uint32_t(crc32(crc32(0,nullptr,0),static_cast<Bytef*>(output),r.raw))==r.crc;break;}
   if(result!=Z_OK||!z.avail_out||(!z.avail_in&&!left)||(beforeIn==z.total_in&&beforeOut==z.total_out))break;
  }
  inflateEnd(&z);workspacePeak=std::max(workspacePeak,unsigned(arena.used));error=good?0:-4;return good;
 }
};
}
