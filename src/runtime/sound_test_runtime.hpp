#pragma once
#include "sound_test_state.hpp"
#include "sound_test_art.hpp"
#include <cstddef>
namespace SoundTest054 {
namespace AudioData=ucg::sound_test_audio;
namespace Art=SoundTestArt;
SoundTestState::State state;
Vec2 pointer{512,256};
float scroll=0,grabOffset=0,animationAge=1,clockSeconds=0,cdSeconds=0,speakerSeconds=0,twinkle=0;
float magnitudes[8]{},fallSpeeds[8]{},dancerSeconds=0;
unsigned dancerFrame=0,lastBeat=0;
volatile unsigned playbackFrame=0;
int captured=-1;
bool heldPrimary=false,dragging=false;
#ifdef UCG_VICTORY_PROBE
extern const volatile uint32_t ProbeLayout[] __attribute__((used))={
 offsetof(SoundTestState::State,selected),offsetof(SoundTestState::State,paused),
 offsetof(SoundTestState::State,visible),offsetof(SoundTestState::State,mix)};
#endif
void event(const char*log,const char*name,unsigned a=0,unsigned b=0){
 if(auto*f=std::fopen(log,"a")){std::fprintf(f,"event=sound_test_%s a=%u b=%u\n",name,a,b);std::fclose(f);}
}
Sound source(){return state.selected>=0?static_cast<Sound>(AudioData::tracks[state.selected].id):Sound::Count;}
void tune(){audio.musicTune(state.paused?0:1,state.paused?0:state.mix?.75f:1.f);}
bool enter(const char*executable,const char*log){
 titleArt=nullptr;titleReusedCourseVram=true;
 slotArt.unload();ashArt.unload();darkHoleArt.unload();dogArt.unload();
 factoryDoors.unload();factoryBalloons.unload();factorySpawner.unload();factoryDecor.unload();
 marcySpillArt.unload();marcyHudArt.unload();marcyTextArt.unload();parkBlockArt.unload();
 wizardArt516.unload();spellArt516.unload();
 state={};const auto flags=Villa054::earned;
 state.gates(VillaUnlocks::unlocked(flags,VillaUnlocks::Id::SongsFields),
             VillaUnlocks::unlocked(flags,VillaUnlocks::Id::SongsCosmic),
             VillaUnlocks::unlocked(flags,VillaUnlocks::Id::Nothing));
 scroll=grabOffset=clockSeconds=cdSeconds=speakerSeconds=twinkle=dancerSeconds=0;
 playbackFrame=0;animationAge=1;captured=-1;heldPrimary=dragging=false;dancerFrame=lastBeat=0;
 std::fill_n(magnitudes,8,0.f);std::fill_n(fallSpeeds,8,0.f);
 audio.muteMusic();audio.musicTune(1,1);audio.observeSoundTest(true);
 fallbackPageIndex=ScreenPages;horseMeter039.invalidateShared();
 if(!Art::load(executable,fallbackPage,sizeof fallbackPage) || !Art::selectCostume(static_cast<unsigned>(Costumes054::selected),ArtistCopycat::base)){
  Art::unload();audio.observeSoundTest(false);event(log,"load_failed");return false;
 }
#ifdef UCG_VICTORY_PROBE
 event(log,"probe_layout",ProbeLayout[0],ProbeLayout[3]);
#endif
 event(log,"open",std::count(state.unlocked.begin(),state.unlocked.end(),true));return true;
}
void leave(){audio.observeSoundTest(false);audio.musicTune(1,1);audio.muteMusic();Art::unload();}
bool choose(unsigned row,const char*log){
 if(row>=58 || !state.unlocked[row])return false;
 if(!Art::select(row)){event(log,"texture_error",static_cast<unsigned>(Art::lastInflateError),Art::lastInflateRecord);logMemory(log,"sound_test_texture_failure",audio.effectMemoryBytes());event(log,"load_failed",row);return false;}
 const bool wasPaused=state.paused;
 if(!state.select(row))return false;
 if(wasPaused)dancerSeconds=0;
 speakerSeconds=0;
 audio.musicAt(static_cast<Sound>(AudioData::tracks[row].id),0);
 tune();animationAge=1;
 event(log,"play",row,AudioData::tracks[row].id);return true;
}
int target(){
 if(Art::BackRect.contains(pointer.x,pointer.y))return 100;
 if(state.visible && Art::PauseRect.contains(pointer.x,pointer.y))return 101;
 if(Art::MixRect.contains(pointer.x,pointer.y))return 102;
 if(Art::ScrollRect.contains(pointer.x,pointer.y))return 103;
 if(Art::ListRect.contains(pointer.x,pointer.y)){
  for(unsigned i=0;i<58;++i)if(state.unlocked[i] && Art::rowButton(i,scroll).contains(pointer.x,pointer.y))return i;
 }
 return -1;
}
void movePointer(Vec2 motion,uint64_t us){
 const float magnitude=length(motion);if(magnitude<=.18f)return;
 const float ratio=(std::min(1.f,magnitude)-.18f)/.82f;
 const auto movement=motion*(400*ratio*std::min<uint64_t>(us,100000)/(1000000*magnitude));
 pointer.x=std::clamp(pointer.x+movement.x,0.f,767.f);pointer.y=std::clamp(pointer.y+movement.y,0.f,511.f);
}
void pauseDown(const char*log){
 if(!state.pauseDown())return;
 if(state.paused)speakerSeconds=0;
 else dancerSeconds=0;
 tune();animationAge=0;event(log,state.paused?"pause":"resume",state.selected);
}
void step(Flow&flow,Vec2 motion,unsigned held,unsigned pressed,unsigned released,uint64_t us,const char*log){
 const float dt=std::min<uint64_t>(us,100000)/1000000.f;
 movePointer(motion,us);heldPrimary=held&PSP_CTRL_CROSS;
 state.step(std::min<uint64_t>(us,100000));clockSeconds+=dt;animationAge+=dt;
 const unsigned left=PSP_CTRL_LTRIGGER,right=PSP_CTRL_RTRIGGER;
 if(!dragging)scroll=std::clamp(scroll+(int(bool(held&right))-int(bool(held&left)))*528*dt,0.f,float(Art::ScrollMax));
 if(pressed&PSP_CTRL_CIRCLE){flow.screen=Screen::Villa;event(log,"exit",state.selected+1);return;}
 int hover=target();
 constexpr float thumb=352.f*352/5796;
 if(pressed&PSP_CTRL_CROSS){
  captured=hover;
  if(hover==101)pauseDown(log);
  else if(hover==103){
   const float top=16+scroll/Art::ScrollMax*(352-thumb);
   if(pointer.y>=top && pointer.y<top+thumb){dragging=true;grabOffset=pointer.y-top;}
   else scroll=std::clamp(scroll+(pointer.y<top?-352:352),0.f,float(Art::ScrollMax));
  }
 }
 if(dragging && heldPrimary)scroll=std::clamp((pointer.y-16-grabOffset)/(352-thumb)*Art::ScrollMax,0.f,float(Art::ScrollMax));
 if(released&PSP_CTRL_CROSS){
  const int action=captured;captured=-1;dragging=false;hover=target();
  if(action>=0 && action==hover){
   if(action<58){if(!choose(action,log)){event(log,"load_failed",action);flow.screen=Screen::Villa;return;}}
   else if(action==100){flow.screen=Screen::Villa;event(log,"exit",state.selected+1);return;}
   else if(action==102 && state.toggleMix()){tune();event(log,"mix",state.mix);}
  }
 }
 if(state.selected>=0){
  const auto& track=AudioData::tracks[state.selected];
  if(!state.paused){
   cdSeconds+=dt*(state.mix?.75f:1.f);dancerSeconds+=dt;
   if(state.speakersPlaying)speakerSeconds+=dt*(track.bpm/60/track.barBeats)*(state.mix?.5f:1.f);
   const unsigned frame=audio.musicPositionFrames(source());playbackFrame=frame;
   {
    const unsigned beat=SoundTestState::reportedBeat(frame,track.bpm);
    if(beat!=lastBeat){lastBeat=beat;dancerFrame=(dancerFrame+1)%4;dancerSeconds=SoundTestState::dancerSeek(dancerFrame);}
   }
   if(!track.loop && frame>=track.frames){
    state.finish();speakerSeconds=0;animationAge=0;tune();event(log,"finished",state.selected);
   }
  }
 }
 twinkle=std::max(twinkle*(1-dt*5),audio.soundTestPeak()*1.5f);
 for(unsigned i=0;i<8;++i){
  const float next=audio.soundTestBand(i);
  if(next>magnitudes[i]){magnitudes[i]=next;fallSpeeds[i]=0;}
  else{fallSpeeds[i]+=2*dt;magnitudes[i]-=fallSpeeds[i]*dt;}
 }
}
void draw(){
 beginFrame();
 Art::drawBackground(clockSeconds,Art::speakerFrame(speakerSeconds,1,state.speakersPlaying&&!state.paused),twinkle,magnitudes);
 Art::drawSelection(state.selected,russian());
 const auto pose=Art::dancerPose(dancerSeconds,state.paused);
 Art::drawDancer(pose,state.paused,clockSeconds);
 if(!state.paused && Costumes054::selected==Costumes054::Id::Copycat && ArtistCopycat::drawn)
  Art::drawDancerInk(pose,ArtistCopycat::pixels.data(),ArtistInk::Palette);
 Art::drawControls(state.visible,state.paused,state.mix,Art::cdFrame(cdSeconds),
                  Art::pokerFrame(.15f-state.pokerUs/1000000.f,state.mix),animationAge,russian(),clockSeconds);
 const int hover=target();
 Art::drawList(scroll,state.selected,hover<58?hover:-1,heldPrimary&&captured==hover&&captured>=0&&captured<58?captured:-1,russian(),state.unlocked.data(),false);
 Art::drawScroll(scroll,dragging?2:hover==103?1:0);
 UniqueHud057::drawMaster();
 Art::cursor(pointer.x,pointer.y);endFrame();
}
}
