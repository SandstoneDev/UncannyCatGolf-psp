#pragma once
#include "game.hpp"
#include "editor_input.hpp"
namespace EditorNativeShot069 {
// Main aimInput: D-pad has priority; an untouched nub is exactly zero.
inline Vec2 aim(unsigned held,unsigned lx,unsigned ly) {
 const float x=static_cast<float>(!!(held&EditorInput::Right)-!!(held&EditorInput::Left));
 const float y=static_cast<float>(!!(held&EditorInput::Down)-!!(held&EditorInput::Up));
 if(x!=0||y!=0)return {x,y};
 return {(int(lx)-128)/127.0f,(int(ly)-128)/127.0f};
}
// Main Controls::update rawDrag mapping. Session keeps its settings transform;
// Scene keeps shot gates, buffering and the source-specific shoot policy.
inline Vec2 drag(Vec2 raw,Vec2 stick,bool god) {
 constexpr float deadzone=.18f;
 const float magnitude=length(stick);
 if(magnitude>deadzone) {
  const float power=(std::min(magnitude,1.0f)-deadzone)/(1.0f-deadzone);
  const Vec2 target=stick*(175.f*power/magnitude);
  raw=god?raw+target*(1.f/8):target;
 }else if(!god)raw={};
 return raw;
}
}
