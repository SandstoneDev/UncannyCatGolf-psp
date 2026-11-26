#pragma once
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <limits>
// Equal synchronous actor voices share PCM and a cursor, preserving their sum.
// OW lasts <0.361s: at60Hz at most23 distinct start ticks can overlap.
class BortVoices {
public:
 struct Snapshot {unsigned actors,cohorts,cursor,started,overflow;};
 void play(unsigned n){pending.fetch_add(n,std::memory_order_relaxed);}
 void stop(){pending.store(StopPending,std::memory_order_release);}
 Snapshot snapshot()const{return {actorCount.load(),cohortCount.load(),cursorSum.load(),started.load(),overflow.load()};}
 void mix(const int16_t*pcm,unsigned frames,int32_t*out,unsigned size,bool paused,unsigned gain=256) {
  const unsigned command=pending.exchange(0,std::memory_order_acquire);
  if(command&StopPending) for(auto&v:voices) v={};
  const unsigned count=command&~StopPending;
  if(count && pcm && frames) {
   bool found=false;
   for(auto&v:voices)if(!v.count){v={0,count};found=true;break;}
   if(found)started.fetch_add(count,std::memory_order_relaxed);
   else overflow.fetch_add(1,std::memory_order_relaxed);
  }
  unsigned actors=0,cohorts=0,cursor=0;
  for(auto&v:voices) {
   if(!v.count)continue;
   if(!paused && pcm) {
    const unsigned length=std::min(size,frames-v.position);
    for(unsigned i=0;i<length*2;++i) {
     const int64_t value=int64_t(out[i])+int64_t(pcm[v.position*2+i])*v.count*std::min(gain,256u)/256;
     out[i]=static_cast<int32_t>(std::clamp<int64_t>(value,INT32_MIN,INT32_MAX));
    }
    v.position+=length;
    if(v.position==frames){v.count=0;continue;}
   }
   actors+=v.count;++cohorts;cursor+=v.position;
  }
  actorCount.store(actors,std::memory_order_relaxed);cohortCount.store(cohorts,std::memory_order_relaxed);
  cursorSum.store(cursor,std::memory_order_relaxed);
 }
private:
 struct Voice{unsigned position=0,count=0;};
 Voice voices[32]{};
 // One exchange orders the stop and all starts after it. Hits are bounded
 // by allocated actors, and each new round waits for mixer completion.
 static_assert(sizeof(unsigned)==4 && std::atomic<unsigned>::is_always_lock_free);
 static constexpr unsigned StopPending=1u<<31;
 std::atomic<unsigned> pending{0},actorCount{0},cohortCount{0},cursorSum{0},started{0},overflow{0};
};
