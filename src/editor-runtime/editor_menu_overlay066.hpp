#pragma once
#include <array>
#include <cstdint>
#include <cstdio>
namespace EditorMenuOverlay066 {
struct Record {uint32_t offset,bytes,raw,format,crc;};
struct Entry {uint32_t id;Record record;};
static_assert(sizeof(Record)==20&&sizeof(Entry)==24,"Editor menu overlay ABI");
class Archive {
  FILE* file_=nullptr;
  std::array<Entry,5> entries_{};
  unsigned count_=0;
public:
  Archive()=default;
  Archive(const Archive&)=delete;
  Archive&operator=(const Archive&)=delete;
  ~Archive(){close();}
  void close(){if(file_)std::fclose(file_);file_=nullptr;entries_={};count_=0;}
  bool active()const{return file_!=nullptr;}
  FILE*file()const{return file_;}
  bool openOwned(FILE*input){
    close();file_=input;
    auto reject=[&](){close();return false;};
    uint32_t h[4]{};
    if(!file_||std::fseek(file_,0,SEEK_SET)||std::fread(h,sizeof h,1,file_)!=1||
       h[0]!=0x364d4355u||h[3]!=24||
       !((h[1]==1&&h[2]==3)||(h[1]==2&&h[2]==5)))return reject();
    count_=h[2];
    if(std::fread(entries_.data(),sizeof(Entry)*count_,1,file_)!=1||std::fseek(file_,0,SEEK_END))return reject();
    const long length=std::ftell(file_);
    if(length<0||uint64_t(length)>16u+count_*24u+count_*262144u)return reject();
    uint64_t expected=16u+count_*24u;
    constexpr unsigned extendedIds[]={13,14,20,21,22};
    for(unsigned i=0;i<count_;++i){
      const auto&e=entries_[i];const auto&r=e.record;
      if(e.id!=(h[1]==1?20+i:extendedIds[i])||r.offset!=expected||!r.bytes||r.bytes>262144||
         r.raw!=262144||r.format!=3||uint64_t(r.offset)+r.bytes>uint64_t(length))return reject();
      expected+=r.bytes;
    }
    if(expected!=uint64_t(length))return reject();
    return true;
  }
  const Record*find(unsigned id)const{
    if(file_)for(unsigned i=0;i<count_;++i)if(entries_[i].id==id)return &entries_[i].record;
    return nullptr;
  }
};
}
