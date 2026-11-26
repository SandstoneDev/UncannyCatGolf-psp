#pragma once
#include "editor_builder_gift_source_data.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iterator>
#include <zlib.h>
namespace EditorBuilderGiftSource {
inline constexpr const char* BankPath=EditorBuilderGiftSourceData::BankPath;
struct Workspace{uint32_t palette[256],xs[256],ys[256];uint8_t compressed[2048],row[1280];};
static_assert(sizeof(Workspace)==6400,"Gift row workspace must stay bounded");
class Reader{
 FILE* file=nullptr;
 const EditorBuilderGiftSourceData::Image* image=nullptr;
 struct Arena{uint8_t* memory;std::size_t bytes,used=0;};
 static voidpf allocate(voidpf opaque,uInt count,uInt size){
  auto& arena=*static_cast<Arena*>(opaque);if(!count||!size||count>arena.bytes/size)return nullptr;
  const std::size_t bytes=std::size_t(count)*size,at=(arena.used+63)&~std::size_t(63);
  if(at>arena.bytes||bytes>arena.bytes-at)return nullptr;
  auto*p=arena.memory+at;std::memset(p,0,bytes);arena.used=at+bytes;return p;
 }
 static void release(voidpf,voidpf){}
public:
 unsigned workspacePeak=0;
 Reader()=default;Reader(const Reader&)=delete;Reader&operator=(const Reader&)=delete;
 ~Reader(){close();}
 void close(){if(file)std::fclose(file);file=nullptr;image=nullptr;}
 bool open(const char* path,unsigned asset,uint32_t* palette){
  close();if(!path||!palette||asset>=std::size(EditorBuilderGiftSourceData::Images))return false;
  file=std::fopen(path,"rb");if(!file)return false;
  uint32_t h[4]{};bool good=std::fread(h,sizeof h,1,file)==1&&h[0]==0x58534255&&h[1]==1&&h[2]==std::size(EditorBuilderGiftSourceData::Images)&&h[3]==EditorBuilderGiftSourceData::BankBytes;
  for(const auto& expected:EditorBuilderGiftSourceData::Images){uint32_t words[6]{};if(std::fread(words,sizeof words,1,file)!=1||words[0]!=expected.width||words[1]!=expected.height||words[2]!=expected.colors||words[3]!=expected.paletteOffset||words[4]!=expected.paletteCRC||words[5]!=expected.rowsOffset)good=false;}
  if(!good||std::fseek(file,0,SEEK_END)||std::ftell(file)!=long(EditorBuilderGiftSourceData::BankBytes)){close();return false;}
  image=&EditorBuilderGiftSourceData::Images[asset];const unsigned bytes=image->colors*4;
  if(std::fseek(file,long(image->paletteOffset),SEEK_SET)||std::fread(palette,1,bytes,file)!=bytes||crc32(crc32(0,nullptr,0),reinterpret_cast<const Bytef*>(palette),bytes)!=image->paletteCRC){close();return false;}
  return true;
 }
 bool readRow(unsigned y,Workspace& workspace,void* memory,std::size_t memoryBytes){
  if(!file||!image||y>=image->height||!memory)return false;
  const uintptr_t raw=reinterpret_cast<uintptr_t>(memory),aligned=(raw+63)&~uintptr_t(63);
  if(raw>UINTPTR_MAX-63||memoryBytes<aligned-raw)return false;
  memoryBytes-=aligned-raw;if(memoryBytes<40000)return false;
  uint32_t record[3]{};
  if(std::fseek(file,long(image->rowsOffset+y*12),SEEK_SET)||std::fread(record,sizeof record,1,file)!=1||!record[1]||record[1]>sizeof workspace.compressed||record[0]<EditorBuilderGiftSourceData::PayloadBegin||record[0]>EditorBuilderGiftSourceData::BankBytes||record[1]>EditorBuilderGiftSourceData::BankBytes-record[0]||std::fseek(file,long(record[0]),SEEK_SET)||std::fread(workspace.compressed,1,record[1],file)!=record[1])return false;
  Arena arena{reinterpret_cast<uint8_t*>(aligned),memoryBytes};z_stream z{};z.zalloc=allocate;z.zfree=release;z.opaque=&arena;
  if(inflateInit(&z)!=Z_OK)return false;
  z.next_in=workspace.compressed;z.avail_in=record[1];z.next_out=workspace.row;z.avail_out=image->width;
  const int result=inflate(&z,Z_FINISH);
  bool good=result==Z_STREAM_END&&z.total_out==image->width&&!z.avail_in&&crc32(crc32(0,nullptr,0),workspace.row,image->width)==record[2];
  inflateEnd(&z);workspacePeak=std::max(workspacePeak,unsigned(arena.used));
  if(good)for(unsigned x=0;x<image->width;++x)if(workspace.row[x]>=image->colors){good=false;break;}
  return good;
 }
};
}
