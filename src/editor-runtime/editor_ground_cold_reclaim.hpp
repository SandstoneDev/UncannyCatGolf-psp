#pragma once
// No GPU pointer is retained here: the existing ground retirement callback
// invokes this only after successful publication and completed-frame fencing.
namespace EditorGroundCold {
struct Result {unsigned entries=0,bytes=0;};
template<class Cache> Result retire(Cache& cache,unsigned builtinId) {
  Result result;
  if(builtinId==~0u)return result;
  for(auto&entry:cache.entries) {
    if(!entry.blocks||&entry==cache.pinned||entry.id!=builtinId||
       (entry.file&&*entry.file)||entry.format!=3||entry.rawBytes!=262144)
      continue;
    ++result.entries;result.bytes+=entry.allocatedBytes;
    cache.erase(entry);
  }
  return result;
}
}
