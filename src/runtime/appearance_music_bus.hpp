#pragma once
#include "user_audio_buses.hpp"
#include <algorithm>
#include <cstdint>

namespace AppearanceMusicBus {
// Mixer-thread scratch only. Capture once after old ordinary effects, before
// the last two appearance voices. No read/copy/delta pass at default Unity.
inline void capture(uint32_t buses, bool appearance, const int32_t* work,
                    int32_t* baseline, unsigned samples, bool& captured) {
    if (buses == UserAudioBuses::Unity || !appearance || captured) return;
    std::copy(work, work + samples, baseline);
    captured = true;
}
// Freeze the appearance-only delta BEFORE late Bort/ring Sounds producers.
inline void finish(const int32_t* work, int32_t* baseline, unsigned samples,
                   bool captured) {
    if (!captured) return;
    for (unsigned i = 0; i < samples; ++i) baseline[i] = work[i] - baseline[i];
}
inline int32_t musicSample(int32_t original, int32_t appearance, bool captured) {
    return original + (captured ? appearance : 0);
}
}
