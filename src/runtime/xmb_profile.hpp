#pragma once
#include "native_game074.hpp"
// Compatibility at the original menu I/O boundary. The shared archive revision
// covers global rewards and other profiles as well as campaign progress.
class XmbProfile {
public:
 struct Timing{unsigned elapsed=0,calls=0,callMax=0;int result=0;};
 void queue(unsigned,const ProfileEntry&,const Progress&){}
 bool forget(unsigned){return !busy();}
 bool busy()const{return NativeGame074::active;}
 bool waiting()const{return NativeGame074::dirty()&&!NativeGame074::failed;}
 bool takeTiming(Timing&out){if(!timingReady)return false;out=timing;timingReady=false;return true;}
 void update(bool allowStart,const char*,const Game&){
  if(!allowStart||!waiting())return;
  const unsigned start=sceKernelGetSystemTimeLow();
  NativeGame074::controller.configure(profileArt.saveIcon(),profileArt.saveIconSize(),nullptr,nullptr);
  NativeGame074::requireFlush();
  const unsigned elapsed=sceKernelGetSystemTimeLow()-start;
  timing={elapsed,1,elapsed,NativeGame074::controller.error()};timingReady=true;
 }
private: Timing timing{};bool timingReady=false;
} xmbProfile;
