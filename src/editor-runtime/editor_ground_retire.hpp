#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
namespace EditorGroundRetire {
inline constexpr unsigned MaxPages=96,RawBytes=256*256*4,Rgba8888=3;
struct Epoch{uint64_t scene=0,document=0,assets=0;unsigned renderer=0;bool operator==(const Epoch&b)const{return scene==b.scene&&document==b.document&&assets==b.assets&&renderer==b.renderer;}};
struct Info{uintptr_t address=0;unsigned id=~0u,age=0,bytes=0,format=0;bool immutableBuiltin=false;};
using Snapshot=bool(*)(void*,unsigned,Info&);
using Discard=void(*)(void*,unsigned);
struct Result{unsigned pages=0,bytes=0;};
struct Stats{uint64_t pages=0,bytes=0,attempts=0,zeroAttempts=0;};

// Captured addresses are identity integers only; no borrowed Page/pixel pointer
// is dereferenced or retained for rendering. Callbacks run on the render thread.
class Batch {
 struct Record{unsigned slot=0;Info info{};};
 std::array<Record,MaxPages> records{};
 Epoch owner{},lastAttempt{};
 uint64_t version=1;
 unsigned beginClock=0,endClock=0,count=0;
 bool armed=false,consumed=false;
 Stats counters{};
 static bool eligible(const Info&v,unsigned sourceRecords){return v.address&&v.immutableBuiltin&&v.id<sourceRecords&&v.format==Rgba8888&&v.bytes==RawBytes;}
 static bool same(const Info&a,const Info&b){return a.address==b.address&&a.id==b.id&&a.age==b.age&&a.bytes==b.bytes&&a.format==b.format&&a.immutableBuiltin==b.immutableBuiltin;}
public:
 static constexpr size_t statsOffset(){return offsetof(Batch,counters);}
 const Stats&stats()const{return counters;}
 unsigned pending()const{return armed?count:0;}
 bool attempted(const Epoch&e)const{return consumed&&lastAttempt==e;}
 void cancel(){armed=false;count=0;if(!++version)++version;}
 unsigned stage(unsigned begin,unsigned end,unsigned pageCount,const Epoch&epoch,unsigned sourceRecords,bool captureSucceeded,Snapshot snapshot,void*context){
  cancel();
  if(attempted(epoch)||!captureSucceeded||!snapshot||!epoch.scene||!epoch.document||!epoch.assets||!sourceRecords||pageCount>MaxPages||end<begin)return 0;
  owner=epoch;beginClock=begin;endClock=end;const uint64_t started=version;
  for(unsigned slot=0;slot<pageCount;++slot){Info v;const bool available=snapshot(context,slot,v);if(started!=version){cancel();return 0;}if(!available)continue;if(eligible(v,sourceRecords)&&v.age>beginClock&&v.age<=endClock)records[count++]={slot,v};}
  armed=true;return count;
 }
 Result finish(bool frameFenced,bool newlyPublished,const Epoch&current,unsigned currentClock,Snapshot snapshot,Discard discard,void*context){
  if(!armed)return {};
  if(!frameFenced||!newlyPublished||!(owner==current)||currentClock<endClock||!snapshot||!discard){cancel();return {};}
  const unsigned n=count;const uint64_t started=version;armed=false;count=0;Result result;
  for(unsigned i=0;i<n;++i){if(started!=version)break;const Record r=records[i];Info now;const bool available=snapshot(context,r.slot,now);if(started!=version)break;if(!available)continue;if(!same(now,r.info))continue;discard(context,r.slot);++result.pages;result.bytes+=r.info.bytes;++counters.pages;counters.bytes+=r.info.bytes;}
  if(started==version){++counters.attempts;if(result.pages){lastAttempt=current;consumed=true;}else ++counters.zeroAttempts;}
  return result;
 }
};
static_assert(sizeof(Batch)<=4096,"fixed bounded retirement metadata only");
}
