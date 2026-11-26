#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace EditorTextFlow068 {
// The caller supplies monotonic milliseconds. Keep both ends still long enough
// to read; never change the stored value or shrink a long name to fit a field.
inline uint32_t tick=0;inline bool outline=false;
inline float offset(float width,float available){
 if(!std::isfinite(width)||!std::isfinite(available)||available<=0||width<=available)return 0;
 constexpr float pause=1000,step=.06f;
 const float distance=width-available,travel=std::ceil(distance/step);
 const float at=std::fmod(float(tick),2*pause+travel);
 return std::clamp((at-pause)*step,0.f,distance);
}
}
