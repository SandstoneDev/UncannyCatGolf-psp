#pragma once
#include "editor_sample_cache.hpp"
namespace EditorSampleHash {
// This is only a lookup prefilter. Cache equality still compares every key
// field and every original plan word, including on forced hash collisions.
inline uint32_t words(void*,const EditorSampleCache::Key&key,unsigned width,
                      unsigned height,const uint32_t*plan,size_t count){
  uint32_t hash=2166136261u;
  auto mix=[&](uint32_t word){hash=(hash^word)*16777619u;};
  mix(uint32_t(key.kind));mix(width);mix(height);
  if(key.kind==EditorSampleCache::Kind::Bank)mix(key.pageId);
  else{
    mix(uint32_t(key.nameBytes));mix(key.x);mix(key.y);mix(key.w);mix(key.h);
    size_t i=0;
    for(;i+4<=key.nameBytes;i+=4){uint32_t word;std::memcpy(&word,key.name+i,4);mix(word);}
    for(;i<key.nameBytes;++i)mix(uint8_t(key.name[i]));
  }
  for(size_t i=0;i<count;++i)mix(plan[i]);
  return hash;
}
}
