#pragma once
#include "sound_test_audio.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <cmath>
namespace SoundTestState {
inline unsigned reportedBeat(unsigned frame,float bpm){return static_cast<unsigned>(std::round(frame/44100.f*bpm*2/60+.5f));}
inline float dancerSeek(unsigned frame){return .2f*((frame+3)%4);}
struct State {
 std::array<bool,58> unlocked{};
 int selected=-1;
 bool paused=true,visible=false,mix=false,speakersPlaying=false;
 unsigned generation=0;
 uint64_t pokerUs=0;
 void gates(bool fields,bool cosmic,bool nothing) {
  for(unsigned i=0;i<58;++i){
   const char*g=ucg::sound_test_audio::tracks[i].unlockGate;
   unlocked[i]=!*g || (!std::strcmp(g,"songs_fields") && fields) ||
    (!std::strcmp(g,"songs_cosmic") && cosmic) || (!std::strcmp(g,"costume_nothing") && nothing);
  }
 }
 bool select(unsigned row) {
  if(row>=58 || !unlocked[row])return false;
  selected=static_cast<int>(row);paused=false;visible=true;speakersPlaying=true;++generation;return true;
 }
 bool pauseDown(){if(!visible)return false;paused=!paused;if(paused)speakersPlaying=false;return true;}
 bool toggleMix(){if(pokerUs)return false;mix=!mix;pokerUs=150000;return true;}
 void step(uint64_t us){pokerUs=us>=pokerUs?0:pokerUs-us;}
 void finish(){paused=true;visible=false;speakersPlaying=false;}
};
}
