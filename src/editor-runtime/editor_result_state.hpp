#pragma once
#include <algorithm>
#include <cstdint>
#include <initializer_list>
namespace EditorPlayHud {
enum class Command { None,Continue,Restart,Resume,Settings,Skip,Exit };
// Level.gd Click skips each authored animation separately, then progresses.
// ResultsSlideDown=2s, ResultsAppear=2.4s, ResultsRank=.1s.
struct State {
  enum class Mode { Playing,Paused,Dead,Won };
  Mode mode=Mode::Playing;
  unsigned elapsed=0,choice=0,soundMask=0,heardRank=0;
  unsigned prismStarted[5]{};
  bool armed=false;
  void reset(){mode=Mode::Playing;elapsed=choice=soundMask=heardRank=0;std::fill_n(prismStarted,5,0u);armed=false;}
  Command update(bool won,bool dead,bool paused,unsigned dt,bool confirm,bool restart=false,unsigned pauseChoice=0){
    const auto next=won?Mode::Won:dead?Mode::Dead:paused?Mode::Paused:Mode::Playing;
    const bool changed=next!=mode;
    const unsigned before=changed?0:elapsed;soundMask=0;
    if(changed){mode=next;elapsed=heardRank=0;std::fill_n(prismStarted,5,0u);armed=false;}
    else elapsed=std::min(0x7fffffffu,elapsed+std::min(dt,1000u));
    choice=std::min(pauseChoice,3u);
    if(won){unsigned bit=1;for(unsigned time:{2005u,2355u,2705u,3050u,4400u}){if(before<time&&elapsed>=time)soundMask|=bit;bit<<=1;}}
    if(restart&&(won||dead||!paused))return Command::Restart;
    if(!confirm||changed)return Command::None;
    if(mode==Mode::Dead)return Command::Restart;
    if(mode==Mode::Won){
      if(elapsed<2000)elapsed=2000;
      else if(elapsed<4400){elapsed=4400;soundMask|=16;}
      else if(elapsed>=4500)return Command::Continue;
    }
    if(mode==Mode::Paused){constexpr Command actions[]={Command::Resume,Command::Settings,Command::Exit,Command::Exit};return actions[choice];}
    return Command::None;
  }
};
} // namespace EditorPlayHud
