#pragma once
#include <algorithm>
#include <cstdint>
namespace EditorTiming {
// Source Godot physics is60Hz independently of draw rate. Eight catch-up
// steps is its default per-frame ceiling; pause/load reset discards stale time.
struct Frame {unsigned physics=0,milliseconds=0;};
struct FixedStep {
 static constexpr unsigned Rate=60,MaxSteps=8;
 uint64_t previous=0,phase=0;uint64_t uiPhase=0;bool initialized=false;
 void reset(){previous=phase=0;uiPhase=0;initialized=false;}
 Frame advance(uint64_t now,bool paused=false){
  if(!initialized||now<previous){previous=now;phase=0;uiPhase=0;initialized=true;return {0,0};}
  const uint64_t elapsed=std::min<uint64_t>(now-previous,1000000);previous=now;
  const uint64_t fullPhase=phase+elapsed*Rate;const unsigned due=unsigned(fullPhase/1000000);
  // Godot main.cpp reduces process delta by the physics steps it drops,
  // so a slow synchronous read cannot jump UI timers by a full second.
  uint64_t processPhase=elapsed*Rate;
  if(due>MaxSteps)processPhase-=uint64_t(due-MaxSteps)*1000000;
  uiPhase+=processPhase;const unsigned milliseconds=unsigned(uiPhase/(Rate*1000));uiPhase%=Rate*1000;
  phase=paused?0:fullPhase%1000000;
  return {paused?0u:std::min(due,MaxSteps),milliseconds};
 }
};
}
