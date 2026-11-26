#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
namespace EditorSourceRows {
inline constexpr unsigned Rows=16,Width=256;
using Getter=bool(*)(void*,unsigned,unsigned,uint32_t*);
struct Stats{uint64_t hits=0,lastHits=0,misses=0,fills=0,evictions=0,failures=0,epochChanges=0,busy=0;};
class Cache{
 struct Entry{uint32_t pixels[Width]{};uint64_t age=0;unsigned page=0,row=0;bool valid=false;};
 Entry entries[Rows]{};
 uint64_t stamp=0,epoch=1;
 unsigned last=Rows;
 bool filling=false;
 Stats data{};
 void touch(Entry& e){
  if(stamp==UINT64_MAX){
   uint64_t rank[Rows]{};uint64_t maximum=0;
   for(unsigned i=0;i<Rows;++i)if(entries[i].valid){rank[i]=1;for(const auto& other:entries)if(other.valid&&other.age<entries[i].age)++rank[i];if(rank[i]>maximum)maximum=rank[i];}
   for(unsigned i=0;i<Rows;++i){if(entries[i].valid)entries[i].age=rank[i];}stamp=maximum;
  }
  e.age=++stamp;
 }
public:
 Cache()=default;Cache(const Cache&)=delete;Cache&operator=(const Cache&)=delete;
 static constexpr std::size_t fixedBytes(){return sizeof(Cache);}
 static constexpr std::size_t dynamicBytes(){return 0;}
 const Stats& stats()const{return data;}
 // Getter must fill all256 originalRGBA words. Failure publishes nothing and
 // does not touch victims, LRU stamps or the previous last-row identity.
 const uint32_t* get(unsigned pageId,unsigned sourceV,Getter getter,void* context){
  if(sourceV>=Width){++data.failures;return nullptr;}
  if(filling){++data.busy;return nullptr;}
  if(last<Rows){auto& e=entries[last];if(e.valid&&e.page==pageId&&e.row==sourceV){++data.hits;++data.lastHits;touch(e);return e.pixels;}}
  for(unsigned i=0;i<Rows;++i){auto& e=entries[i];if(e.valid&&e.page==pageId&&e.row==sourceV){++data.hits;touch(e);last=i;return e.pixels;}}
  ++data.misses;if(!getter){++data.failures;return nullptr;}
  uint32_t candidate[Width];const uint64_t before=epoch;filling=true;
  const bool good=getter(context,pageId,sourceV,candidate);filling=false;
  if(before!=epoch){++data.epochChanges;return nullptr;}
  if(!good){++data.failures;return nullptr;}
  unsigned chosen=Rows;uint64_t oldest=UINT64_MAX;
  for(unsigned i=0;i<Rows;++i){if(!entries[i].valid){chosen=i;break;}if(entries[i].age<oldest){oldest=entries[i].age;chosen=i;}}
  auto& e=entries[chosen];if(e.valid)++data.evictions;
  std::memcpy(e.pixels,candidate,sizeof candidate);e.page=pageId;e.row=sourceV;e.valid=true;touch(e);last=chosen;++data.fills;return e.pixels;
 }
 void clear(){for(auto& e:entries)e.valid=false;last=Rows;stamp=0;++epoch;}
};
static_assert(Cache::fixedBytes()<=17*1024,"Source rows must remain within17KiB");
}
