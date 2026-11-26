#pragma once
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
namespace EditorMemory {
// Check the actual largest contiguous allocation and retain the rest in
// bounded chunks. Tree copies consist of many independent allocations.
template<class Allocate,class Release>
bool probe(size_t total,size_t largest,size_t largeBlocks,Allocate allocate,Release release){
 constexpr size_t Chunk=64*1024;
 std::array<void*,512> retained{};size_t count=0,remaining=total;
 if(largest>remaining||remaining>Chunk*(retained.size()-1))return false;
 const size_t firstCount=largest?std::max(size_t(1),largeBlocks):0;
 if(firstCount>retained.size()||(largest&&firstCount>total/largest))return false;
 const size_t tail=total-firstCount*largest;
 if(firstCount+(tail+Chunk-1)/Chunk>retained.size())return false;
 size_t next=largest?largest:std::min(Chunk,remaining);bool passed=true;
 size_t initial=firstCount;
 while(remaining){
  void*p=allocate(next);
  if(!p){passed=false;break;}
  // PSP GCC -O2 removes an unused malloc/free capacity check entirely.
  auto*observed=static_cast<volatile uint8_t*>(p);
  observed[0]=0;observed[next-1]=0;
  retained[count++]=p;remaining-=next;if(initial)--initial;
  next=initial?largest:std::min(Chunk,remaining);
 }
 while(count)release(retained[--count]);
 return passed;
}
inline bool probe(size_t total,size_t largest,size_t largeBlocks=1){
 return probe(total,largest,largeBlocks,[](size_t bytes){return std::malloc(bytes);},[](void*p){std::free(p);});
}
}
