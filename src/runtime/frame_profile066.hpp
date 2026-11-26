#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
namespace FrameProfile066 {
struct Window {
 uint32_t kind=0,context=0,frames=0,frameMax=0,stepMax=0,renderMax=0,buildMax=0,syncMax=0,actorsMax=0;
 uint64_t frameSum=0,stepSum=0,renderSum=0,buildSum=0,syncSum=0;
};
struct State {
 std::array<Window,32> records{};Window pending{};unsigned first=0,count=0,dropped=0;
 bool enabled=false,wasActive=false;uint32_t buildStamp=0,syncStamp=0,buildUs=0,syncUs=0;
 bool setActive(bool active){const bool flush=wasActive&&!active;wasActive=enabled=active;return flush;}
 void finish(){if(!pending.frames)return;if(count==records.size()){first=(first+1)%records.size();--count;++dropped;}records[(first+count++)%records.size()]=pending;pending={};}
 void observe(uint32_t kind,uint32_t context,uint32_t frame,uint32_t step,uint32_t render,uint32_t build,uint32_t sync,uint32_t actors){
  if(pending.frames&&(pending.kind!=kind||pending.context!=context))finish();
  auto&w=pending;w.kind=kind;w.context=context;++w.frames;w.frameSum+=frame;w.stepSum+=step;w.renderSum+=render;w.buildSum+=build;w.syncSum+=sync;
  w.frameMax=std::max(w.frameMax,frame);w.stepMax=std::max(w.stepMax,step);w.renderMax=std::max(w.renderMax,render);w.buildMax=std::max(w.buildMax,build);w.syncMax=std::max(w.syncMax,sync);w.actorsMax=std::max(w.actorsMax,actors);
  if(w.frames==120)finish();
 }
 void begin(uint32_t stamp){buildStamp=stamp;}
 void built(uint32_t stamp){buildUs+=stamp-buildStamp;syncStamp=stamp;}
 void synced(uint32_t stamp){syncUs+=stamp-syncStamp;}
 bool dump(FILE*out){
  finish();bool good=out!=nullptr;
  for(unsigned i=0;good&&i<count;++i){const auto&w=records[(first+i)%records.size()];
   good=std::fprintf(out,"build=066 kind=%u context=%u frames=%u frame_sum_us=%llu frame_max_us=%u step_sum_us=%llu step_max_us=%u render_sum_us=%llu render_max_us=%u cpu_build_sum_us=%llu cpu_build_max_us=%u ge_sync_sum_us=%llu ge_sync_max_us=%u actors_max=%u dropped=%u\n",unsigned(w.kind),unsigned(w.context),unsigned(w.frames),(unsigned long long)w.frameSum,unsigned(w.frameMax),(unsigned long long)w.stepSum,unsigned(w.stepMax),(unsigned long long)w.renderSum,unsigned(w.renderMax),(unsigned long long)w.buildSum,unsigned(w.buildMax),(unsigned long long)w.syncSum,unsigned(w.syncMax),unsigned(w.actorsMax),dropped)>=0;
  }return good;
 }
 void clear(){first=count=dropped=0;pending={};}
};
}
