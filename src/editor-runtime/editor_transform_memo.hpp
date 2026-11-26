#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include <type_traits>
namespace EditorTransformMemo {
// Invocation-local memo for one unchanged pure transform. Full input bits are
// compared; overflow only evicts, never approximates a coordinate or result.
template<class Point,class Value,unsigned Capacity=16>class Cache {
 static_assert(Capacity>0&&Capacity<=32,"bounded stack memo");
 static_assert(std::is_trivially_copyable<Point>::value&&std::is_trivially_copyable<Value>::value,"exact value copies");
 struct Entry{std::array<uint8_t,sizeof(Point)>key;Value value;};
 std::array<Entry,Capacity> entries;
 unsigned used=0,next=0;
public:
 template<class Transform>Value get(const Point&point,const Transform&transform){
  std::array<uint8_t,sizeof(Point)>key;std::memcpy(key.data(),&point,sizeof point);
  for(unsigned i=0;i<used;++i)if(!std::memcmp(entries[i].key.data(),key.data(),key.size()))return entries[i].value;
  const Value value=transform(point);
  const unsigned slot=used<Capacity?used++:next++%Capacity;
  entries[slot].key=key;entries[slot].value=value;return value;
 }
};
}
