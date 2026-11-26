#pragma once
#include <algorithm>
#include <cstdint>
namespace UserAudioBuses {
inline constexpr uint32_t pack(unsigned master,unsigned music,unsigned sounds) {
    return (master>20?20:master)|((music>20?20:music)<<5)|((sounds>20?20:sounds)<<10);
}
inline constexpr uint32_t Unity=pack(20,20,20);
// Receives the aggregate after every effect producer. Music is its original
// authored contribution before effects. No producer cursor, pitch or pause changes.
inline int32_t combine(int32_t aggregate,int32_t music,uint32_t steps) {
    if (steps==Unity) return aggregate;
    const int64_t master=steps&31,musicGain=(steps>>5)&31,soundGain=(steps>>10)&31;
    const int64_t value=(int64_t(music)*musicGain+(int64_t(aggregate)-music)*soundGain)*master/400;
    return static_cast<int32_t>(std::clamp<int64_t>(value,INT32_MIN,INT32_MAX));
}
} // namespace UserAudioBuses
