#pragma once
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <zlib.h>
namespace EditorIndexedSource {
enum class Result {Unavailable,Ready,Fault};
using ReadAt=bool(*)(void*,uint32_t,void*,size_t);
inline constexpr unsigned PayloadBytes=66560,WorkspaceBytes=65536;
struct Record {uint32_t offset,bytes,colors,encodedCrc,payloadCrc,sourceCrc,sourceBytes,flags;};
static_assert(sizeof(Record)==32,"indexed source record ABI");
template<unsigned Count> class Reader {
  static_assert(Count>0&&Count<=4096,"bounded indexed page directory");
  std::array<Record,Count> records{};
  ReadAt read=nullptr;void*context=nullptr;uint32_t length=0;
  struct Arena {uint8_t*data;unsigned size,used=0;};
  static uint32_t word(const uint8_t*p){return uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);}
  static bool overlaps(const void*a,unsigned an,const void*b,unsigned bn){const uintptr_t x=reinterpret_cast<uintptr_t>(a),y=reinterpret_cast<uintptr_t>(b);return x<=y?y-x<an:x-y<bn;}
  static voidpf allocate(voidpf opaque,uInt n,uInt size){
    auto&a=*static_cast<Arena*>(opaque);if(!n||!size||n>a.size/size)return nullptr;
    const unsigned bytes=n*size;const uintptr_t base=reinterpret_cast<uintptr_t>(a.data);
    const unsigned at=unsigned(((base+a.used+63)&~uintptr_t(63))-base);
    if(at>a.size||bytes>a.size-at)return nullptr;
    void*p=a.data+at;std::memset(p,0,bytes);a.used=at+bytes;return p;
  }
  static void release(voidpf,voidpf){}
public:
  void close(){read=nullptr;context=nullptr;length=0;}
  bool ready()const{return read!=nullptr;}
  unsigned colors(unsigned page)const{return ready()&&page<Count?records[page].colors:0;}
  bool open(ReadAt getter,void*opaque,uint32_t bytes){
    close();uint8_t header[16];
    if(!getter||bytes<16+uint64_t(Count)*32||!getter(opaque,0,header,sizeof header)||
       word(header)!=0x58494355||word(header+4)!=1||word(header+8)!=Count||word(header+12)!=32)return false;
    uint64_t expected=16+uint64_t(Count)*32;
    uint8_t directory[4096];
    for(unsigned page=0;page<Count;++page){
      if(page%128==0&&!getter(opaque,16+page*32,directory,std::min(128u,Count-page)*32))return false;
      const uint8_t*data=directory+(page%128)*32;
      Record r{word(data),word(data+4),word(data+8),word(data+12),word(data+16),word(data+20),word(data+24),word(data+28)};
      if(!r.bytes){for(unsigned i=0;i<32;++i)if(data[i])return false;records[page]={};continue;}
      if(r.offset!=expected||!r.colors||r.colors>256||r.sourceBytes!=262144||
         (r.flags!=1&&r.flags!=2)||r.bytes>PayloadBytes||
         (r.flags==2&&r.bytes!=PayloadBytes)||
         r.offset>bytes||r.bytes>bytes-r.offset)return false;
      expected+=r.bytes;records[page]=r;
    }
    if(expected!=bytes)return false;
    read=getter;context=opaque;length=bytes;return true;
  }
  Result decode(unsigned page,uint32_t expectedSourceCrc,void*target,unsigned targetBytes,void*workspace,unsigned workspaceBytes){
    if(!read)return Result::Unavailable;
    if(page>=Count)return Result::Fault;
    const auto&r=records[page];if(!r.bytes)return Result::Unavailable;
    if(!target||targetBytes<PayloadBytes||!workspace||workspaceBytes<WorkspaceBytes||
       overlaps(target,PayloadBytes,workspace,workspaceBytes)||r.sourceCrc!=expectedSourceCrc||
       r.offset>length||r.bytes>length-r.offset)return Result::Fault;
    auto*out=static_cast<uint8_t*>(target);uint32_t encodedCrc=0;
    if(r.flags==2){
      if(!read(context,r.offset,out,PayloadBytes))return Result::Fault;
      encodedCrc=uint32_t(crc32(0,out,PayloadBytes));
    }else{
      Arena arena{static_cast<uint8_t*>(workspace),workspaceBytes};z_stream z{};
      z.zalloc=allocate;z.zfree=release;z.opaque=&arena;
      if(inflateInit(&z)!=Z_OK)return Result::Fault;
      uint8_t input[4096],overflow=0;unsigned supplied=0;bool success=false;
      for(;;){
        if(!z.avail_in&&supplied<r.bytes){
          const unsigned n=std::min<unsigned>(sizeof input,r.bytes-supplied);
          if(!read(context,r.offset+supplied,input,n))break;
          encodedCrc=uint32_t(crc32(encodedCrc,input,n));supplied+=n;z.next_in=input;z.avail_in=n;
        }
        z.next_out=z.total_out<PayloadBytes?out+z.total_out:&overflow;
        z.avail_out=z.total_out<PayloadBytes?PayloadBytes-unsigned(z.total_out):1;
        const auto beforeIn=z.total_in,beforeOut=z.total_out;const int status=inflate(&z,Z_NO_FLUSH);
        if(z.total_out>PayloadBytes)break;
        if(status==Z_STREAM_END){success=z.total_out==PayloadBytes&&z.total_in==r.bytes&&supplied==r.bytes&&!z.avail_in;break;}
        if(status!=Z_OK||(beforeIn==z.total_in&&beforeOut==z.total_out))break;
      }
      inflateEnd(&z);if(!success)return Result::Fault;
    }
    if(encodedCrc!=r.encodedCrc||uint32_t(crc32(0,out,PayloadBytes))!=r.payloadCrc)return Result::Fault;
    if(r.colors<256)for(unsigned i=0;i<65536;++i)if(out[i]>=r.colors)return Result::Fault;
    return Result::Ready;
  }
};
}
