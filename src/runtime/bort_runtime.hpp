#pragma once
#include "bort_sim.hpp"
#include "bort_art.hpp"
#include "pause_navigation060.hpp"
#include <cstddef>
namespace Bort054 {
BortSim::Run game;
Vec2 pointer{512,256};
#ifdef UCG_VICTORY_PROBE
extern const volatile uint32_t ProbeLayout[] __attribute__((used))={
 offsetof(BortSim::Run,phase),offsetof(BortSim::Run,round),offsetof(BortSim::Run,left),
 offsetof(BortSim::Run,score),offsetof(BortSim::Run,count),offsetof(BortSim::Run,actors),
 sizeof(BortSim::Actor),offsetof(BortSim::Actor,x),offsetof(BortSim::Actor,y),offsetof(BortSim::Actor,bashed)};
#endif
bool paused=false,pointerHeld=false;
int captured=-1;
unsigned pauseSelected=0;
unsigned pending=0,congratsSerial=0;
uint64_t accumulator=0;
void event(const char*log,const char*name,unsigned a=0,unsigned b=0) {
 if(auto*f=std::fopen(log,"a")){std::fprintf(f,"event=bort_%s a=%u b=%u\n",name,a,b);std::fclose(f);}
}
void stopEffects() {
 audio.stopBortOw();audio.stop(Sound::BortOw);audio.stop(Sound::BortCongrats);
 audio.stop(Sound::BortLose);audio.stop(Sound::BortPause);
}
bool enter(const char*executable,const char*log) {
 titleArt=nullptr;titleReusedCourseVram=true;
 slotArt.unload();ashArt.unload();darkHoleArt.unload();dogArt.unload();
 factoryDoors.unload();factoryBalloons.unload();factorySpawner.unload();factoryDecor.unload();
 marcySpillArt.unload();marcyHudArt.unload();marcyTextArt.unload();parkBlockArt.unload();
 wizardArt516.unload();spellArt516.unload();
 game.reset(sceKernelGetSystemTimeLow()|1u);pointer={512,256};
 paused=pointerHeld=false;captured=-1;pending=0;accumulator=0;
 stopEffects();audio.pauseBort(false);audio.musicTune(1,1);audio.muteMusic();
 if(!BortArt::load(executable)){event(log,"load_failed");return false;}
#ifdef UCG_VICTORY_PROBE
 event(log,"probe_layout",ProbeLayout[0],ProbeLayout[5]);
#endif
 event(log,"open");return true;
}
void leave() {
 stopEffects();audio.pauseBort(false);audio.musicTune(1,1);audio.muteMusic();
 BortArt::unload();game.reset(1);
}
int target() {
 if(paused)return 2+static_cast<int>(pauseSelected);
 if(game.phase==BortSim::Phase::Prompt)return 0;
 if(game.phase==BortSim::Phase::GameOver)return 1;
 return -1;
}
void movePointer(Vec2 motion,uint64_t us) {
 const float magnitude=length(motion);if(magnitude<=.18f)return;
 const float ratio=(std::min(1.f,magnitude)-.18f)/.82f;
 const auto movement=motion*(400*ratio*std::min<uint64_t>(us,100000)/(1000000*magnitude));
 pointer.x=std::clamp(pointer.x+movement.x,0.f,767.f);pointer.y=std::clamp(pointer.y+movement.y,0.f,511.f);
}
void step(Flow&flow,Vec2 motion,unsigned held,unsigned pressed,unsigned released,uint64_t us,const char*log) {
 pointerHeld=held&PSP_CTRL_CROSS;
 if(paused){
  const auto action=PauseNavigation060::update(pauseSelected,captured,
   pressed&PSP_CTRL_UP,pressed&PSP_CTRL_DOWN,pressed&PSP_CTRL_CROSS,
   released&PSP_CTRL_CROSS,pressed&(PSP_CTRL_START|PSP_CTRL_CIRCLE));
  if(action==PauseNavigation060::Action::Resume){
   paused=false;pending=0;accumulator=0;audio.pauseBort(false);audio.musicTune(1,1);event(log,"resume",game.round,game.score);
  }else if(action==PauseNavigation060::Action::Quit){flow.screen=Screen::Villa;event(log,"exit",game.round,game.score);}
  return;
 }
 const bool menu=game.phase==BortSim::Phase::Prompt||game.phase==BortSim::Phase::GameOver;
 if(menu&&(pressed&PSP_CTRL_CIRCLE)){captured=-1;flow.screen=Screen::Villa;event(log,"exit",game.round,game.score);return;}
 if(!menu)movePointer(motion,us);
 if(!paused && (pressed&PSP_CTRL_START)) {
  paused=true;pauseSelected=0;captured=-1;pending=0;accumulator=0;
  audio.pauseBort(true);audio.musicTune(0,0);audio.play(Sound::BortPause);
  event(log,"pause",game.round,game.score);return;
 }
 const int hover=target();
 if(pressed&PSP_CTRL_CROSS)captured=hover;
 if(released&PSP_CTRL_CROSS) {
  const int action=captured;captured=-1;
  if(action>=0 && action==hover) {
   event(log,"click",action);
   if(action==2){paused=false;pending=0;accumulator=0;audio.pauseBort(false);audio.musicTune(1,1);event(log,"resume",game.round,game.score);return;}
   if(action==3){flow.screen=Screen::Villa;event(log,"exit",game.round,game.score);return;}
   if(action==0) {
    if(!game.start()){event(log,"allocation_failed");flow.screen=Screen::Villa;return;}
    audio.music(Sound::BortMusic);pending=0;accumulator=0;
    event(log,"start",game.round,game.left);return;
   }
   if(action==1) {
    stopEffects();audio.muteMusic();game.reset(sceKernelGetSystemTimeLow()|1u);
    pending=0;accumulator=0;event(log,"retry");return;
   }
  }
 }
 if(paused)return;
 if(pressed&PSP_CTRL_CROSS)pending|=1;
 accumulator+=std::min<uint64_t>(us,100000)*60;
 for(;accumulator>=1000000;accumulator-=1000000) {
  game.step(pointer.x,pointer.y,pending!=0);pending=0;
  if(game.cleared) {
   congratsSerial=audio.bortCongratsSerial()+1;audio.play(Sound::BortCongrats);
   event(log,"clear",game.round,game.score);
  }
  if(game.hits){audio.playBortOw(game.hits);event(log,"hit",game.hits,game.score);}
  if(game.lost) {
   audio.stopBortOw();audio.stop(Sound::BortCongrats);audio.muteMusic();audio.play(Sound::BortLose);
   event(log,"lose",game.rank(),game.score);
  }
  if(game.phase==BortSim::Phase::Congrats && audio.bortCongratsSerial()==congratsSerial &&
     audio.bortEffectPosition(Sound::BortCongrats)>=audio.totalFrames(Sound::BortCongrats)) {
   if(!game.finishCongrats()){event(log,"allocation_failed");flow.screen=Screen::Villa;return;}
   event(log,"round",game.round,game.left);
  }
 }
}
void draw() {
 beginFrame();
 BortArt::drawScene(game.round,game.left,game.score,game.seconds(),russian());
 for(unsigned i=0;i<game.count;++i) {
  const auto&a=game.actors[i];if(!a.visible)continue;
  if(a.bashed)BortArt::explosion(a.x,a.y,std::min(16u,static_cast<unsigned>(a.explosionUs*30/1000000)));
  else BortArt::bort(a.x,a.y,a.frame,a.hue);
 }
 const int hover=target();
 if(game.phase==BortSim::Phase::Prompt)BortArt::prompt(russian(),hover==0,pointerHeld&&captured==0&&hover==0);
 if(game.phase==BortSim::Phase::GameOver)BortArt::gameOver(game.rank(),russian(),hover==1,pointerHeld&&captured==1&&hover==1);
 if(paused)BortArt::pause(russian(),pauseSelected,pointerHeld&&captured==static_cast<int>(pauseSelected)?captured:-1);
 if(!paused&&game.phase!=BortSim::Phase::Prompt&&game.phase!=BortSim::Phase::GameOver)BortArt::cursor(pointer.x,pointer.y);
 endFrame();
}
}
