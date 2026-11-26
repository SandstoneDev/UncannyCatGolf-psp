#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
namespace EditorPerf067 {
enum Zone:unsigned { Setup,Floor,Sand,Behind,Tiles,Objects,Aux,Front,Canvas,Preview,Panel,Overlay,Zones };
inline constexpr const char*Names[Zones]={"setup","floor","sand","behind","tiles","objects","aux","front","canvas","preview","panel","overlay"};
// Wall spans may contain synchronous reads/decodes and explicit GE fences.
// build/sync are the exact existing list-timer deltas within the same span;
// build is list-open elapsed time, not a pure CPU instruction measurement.
struct Stamp {uint32_t wall=0,build=0,sync=0;};
struct Metric {uint64_t wall=0,build=0,sync=0;uint32_t wallMax=0,buildMax=0,syncMax=0;};
struct Window {
 unsigned context=0,from=0,frames=0,listMax=0,forcedSplits=0;std::array<Metric,Zones>zone{};
 uint64_t sourceReads=0,customReads=0,coldDecodes=0,artDecodes=0,batches=0,fontReads=0;
};
struct Counters {uint32_t sourceReads=0,customReads=0,coldDecodes=0,artDecodes=0,batches=0,fontReads=0;};
struct State {
 static constexpr unsigned Capacity=16;
 std::array<Window,Capacity>records{};Window pending{};
 std::array<Stamp,Zones>frame{};Counters before{};unsigned from=0,first=0,count=0,dropped=0;
 bool active=false;unsigned listMax=0,forcedSplits=0;
 void begin(unsigned context,Counters counters){frame={};before=counters;from=context;active=true;listMax=forcedSplits=0;}
 void noteList(unsigned bytes){if(active)listMax=std::max(listMax,bytes);}
 void forcedSplit(){if(active)++forcedSplits;}
 void measure(unsigned zone,Stamp a,Stamp b){if(!active||zone>=Zones)return;auto&m=frame[zone];m.wall+=b.wall-a.wall;m.build+=b.build-a.build;m.sync+=b.sync-a.sync;}
 void finish(){if(!pending.frames)return;if(count==Capacity){first=(first+1)%Capacity;--count;++dropped;}records[(first+count++)%Capacity]=pending;pending={};}
 void observe(unsigned context,Counters after){
  active=false;if(pending.frames&&(pending.context!=context||pending.from!=from))finish();
  auto&w=pending;w.context=context;w.from=from;++w.frames;w.listMax=std::max(w.listMax,listMax);w.forcedSplits+=forcedSplits;
  for(unsigned i=0;i<Zones;++i){auto&m=w.zone[i];const auto&f=frame[i];m.wall+=f.wall;m.build+=f.build;m.sync+=f.sync;m.wallMax=std::max(m.wallMax,f.wall);m.buildMax=std::max(m.buildMax,f.build);m.syncMax=std::max(m.syncMax,f.sync);}
  w.sourceReads+=uint32_t(after.sourceReads-before.sourceReads);w.customReads+=uint32_t(after.customReads-before.customReads);w.coldDecodes+=uint32_t(after.coldDecodes-before.coldDecodes);w.artDecodes+=uint32_t(after.artDecodes-before.artDecodes);w.batches+=uint32_t(after.batches-before.batches);w.fontReads+=uint32_t(after.fontReads-before.fontReads);
  if(w.frames==120)finish();
 }
 bool dump(FILE*out){
  finish();if(!out)return false;
  for(unsigned i=0;i<count;++i){const auto&w=records[(first+i)%Capacity];
   if(std::fprintf(out,"build=067 context=%u from=%u frames=%u dropped=%u source_reads=%llu custom_reads=%llu cold_decodes=%llu art_decodes=%llu batches=%llu list_max_bytes=%u forced_splits=%u font_decodes=%llu\n",w.context,w.from,w.frames,dropped,(unsigned long long)w.sourceReads,(unsigned long long)w.customReads,(unsigned long long)w.coldDecodes,(unsigned long long)w.artDecodes,(unsigned long long)w.batches,w.listMax,w.forcedSplits,(unsigned long long)w.fontReads)<0)return false;
   for(unsigned z=0;z<Zones;++z){const auto&m=w.zone[z];if(std::fprintf(out,"zone=%s wall_sum_us=%llu wall_max_us=%u build_sum_us=%llu build_max_us=%u sync_sum_us=%llu sync_max_us=%u\n",Names[z],(unsigned long long)m.wall,unsigned(m.wallMax),(unsigned long long)m.build,unsigned(m.buildMax),(unsigned long long)m.sync,unsigned(m.syncMax))<0)return false;}
  }return true;
 }
 void clear(){first=count=dropped=0;pending={};}
};
static_assert(sizeof(State)<10000,"bounded Editor diagnostics");
}
