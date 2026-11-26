#pragma once
#include <cstdint>
namespace EditorRaster {
// Metadata only: slices belong to the caller's existing 256x256 RGBA buffer.
// Epoch changes are completion certificates: the caller must submit AND fence
// the old GE list before beginning an epoch that may overwrite its slices.
struct Arena {
 static constexpr unsigned Capacity=262144,Alignment=64,DynamicBytes=0;
 struct Stats {
  uint64_t reservations=0,denials=0,epochChanges=0,exclusiveMarks=0,bytesReserved=0;
  unsigned highWater=0;
 } stats;
 uint64_t epoch=0;
 unsigned used=0;
 bool generationValid=false,exclusive=false;
 void resetGeneration(){generationValid=false;used=0;exclusive=false;}
 void generation(uint64_t next){
  if(!generationValid||next!=epoch){epoch=next;generationValid=true;used=0;exclusive=false;++stats.epochChanges;}
 }
 bool reserve(unsigned bytes,uint64_t next,unsigned& offset){
  generation(next);
  if(!bytes||bytes>Capacity||exclusive){++stats.denials;return false;}
  const unsigned size=(bytes+Alignment-1)&~(Alignment-1);
  if(size>Capacity-used){++stats.denials;return false;}
  offset=used;used+=size;++stats.reservations;stats.bytesReserved+=size;
  if(used>stats.highWater)stats.highWater=used;
  return true;
 }
 // The caller has fenced prior readers before borrowing the entire buffer.
 // Its newly queued whole-buffer draw remains exclusive until that list ends.
 void markExclusive(uint64_t next){
  generation(next);if(!exclusive)++stats.exclusiveMarks;exclusive=true;used=Capacity;
  if(used>stats.highWater)stats.highWater=used;
 }
};
}
