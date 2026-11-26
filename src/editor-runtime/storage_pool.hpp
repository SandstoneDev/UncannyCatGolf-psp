#pragma once
#include "storage_reader.hpp"
#include <array>
namespace ucg_storage {
// wait/signal count unpinned windows; lock/unlock protect metadata only.
// The sleeping availability wait occurs before taking the metadata lock.
struct PoolSync {
 bool(*lock)(void*)=nullptr;void(*unlock)(void*)=nullptr;
 bool(*wait)(void*)=nullptr;void(*signal)(void*)=nullptr;void* context=nullptr;
 bool complete()const{return lock&&unlock&&wait&&signal;}
};
template<unsigned Count=4>class WindowPool {
 static_assert(Count>0&&Count<=16,"bounded decoded window pool");
 struct alignas(64) Slot {
  std::array<unsigned char,65536> bytes{};
  const void* owner=nullptr;uint64_t epoch=0,serial=0,age=0;
  bool pinned=false,valid=false;
 };
 std::array<Slot,Count> slots{};PoolSync sync{};bool initialized=false;
 uint64_t serial=0,clock=0;
public:
 struct Stats {unsigned pinned=0,peakPinned=0;uint64_t acquisitions=0,evictions=0,invalidReleases=0;};
private:
 Stats counters{};
 static bool acquireHook(void*c,const void*owner,uint64_t epoch,DecodedWindow&out){return static_cast<WindowPool*>(c)->acquire(owner,epoch,out);}
 static void releaseHook(void*c,const void*owner,uint64_t epoch,const DecodedWindow&window){static_cast<WindowPool*>(c)->release(owner,epoch,window);}
 static void invalidateHook(void*c,const void*owner,uint64_t epoch){static_cast<WindowPool*>(c)->invalidate(owner,epoch);}
 uint64_t nextSerial(){if(!++serial)++serial;return serial;}
public:
 WindowPool()=default;WindowPool(const WindowPool&)=delete;WindowPool&operator=(const WindowPool&)=delete;
 bool initialize(PoolSync hooks){if(initialized||!hooks.complete())return false;sync=hooks;initialized=true;return true;}
 WindowHooks hooks(){return {acquireHook,releaseHook,invalidateHook,this};}
 size_t residentBytes()const{return sizeof(*this);}
 static constexpr unsigned windowBytes(){return Count*65536u;}
 Stats stats(){if(!initialized||!sync.lock(sync.context))return {};const auto result=counters;sync.unlock(sync.context);return result;}
 bool clear(){
  if(!initialized||!sync.lock(sync.context))return false;
  if(counters.pinned){sync.unlock(sync.context);return false;}
  for(auto&slot:slots){slot.owner=nullptr;slot.epoch=0;slot.valid=false;}
  sync.unlock(sync.context);return true;
 }
 bool acquire(const void*owner,uint64_t epoch,DecodedWindow&out){
  out={};if(!initialized||!owner||!epoch||!sync.wait(sync.context))return false;
  if(!sync.lock(sync.context)){sync.signal(sync.context);return false;}
  Slot*chosen=nullptr;
  for(auto&slot:slots)if(!slot.pinned&&slot.valid&&slot.owner==owner&&slot.epoch==epoch){chosen=&slot;break;}
  if(!chosen)for(auto&slot:slots)if(!slot.pinned&&(!chosen||!slot.valid||(chosen->valid&&slot.age<chosen->age))){chosen=&slot;if(!slot.valid)break;}
  if(!chosen){sync.unlock(sync.context);sync.signal(sync.context);return false;}
  if(!chosen->valid||chosen->owner!=owner||chosen->epoch!=epoch){
   if(chosen->valid)++counters.evictions;
   chosen->owner=owner;chosen->epoch=epoch;chosen->serial=nextSerial();chosen->valid=true;
  }
  chosen->pinned=true;if(!++clock)++clock;chosen->age=clock;
  ++counters.acquisitions;++counters.pinned;counters.peakPinned=std::max(counters.peakPinned,counters.pinned);
  out={chosen->bytes.data(),chosen->serial};sync.unlock(sync.context);return true;
 }
 void release(const void*owner,uint64_t epoch,const DecodedWindow&window){
  if(!initialized||!sync.lock(sync.context))return;
  bool released=false;
  for(auto&slot:slots)if(slot.bytes.data()==window.data&&slot.owner==owner&&slot.epoch==epoch&&slot.serial==window.serial&&slot.pinned){
   slot.pinned=false;--counters.pinned;if(!slot.valid){slot.owner=nullptr;slot.epoch=0;}released=true;break;
  }
  if(!released)++counters.invalidReleases;
  sync.unlock(sync.context);if(released)sync.signal(sync.context);
 }
 void invalidate(const void*owner,uint64_t epoch){
  if(!initialized||!sync.lock(sync.context))return;
  for(auto&slot:slots)if(slot.owner==owner&&slot.epoch==epoch){
   // A pinned token retains its owner/epoch/serial until the matching release.
   slot.valid=false;if(!slot.pinned){slot.owner=nullptr;slot.epoch=0;}
  }
  sync.unlock(sync.context);
 }
};
}
