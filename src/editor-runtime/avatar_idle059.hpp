#pragma once
#include <cstdint>
namespace AvatarIdle059 {
struct Context {
 bool active=false,paused=false,spawning=false,moving=false,airborne=false;
 bool parry=false,telekinetic=false,dead=false,won=false,demo=false,cinematic=false;
 bool eligible()const {return active&&!paused&&!spawning&&!moving&&!airborne&&!parry&&!telekinetic&&!dead&&!won&&!demo&&!cinematic;}
};
struct Random {
 uint32_t state=1;
 unsigned roll(){
  // PIdle randi_range(0,100000), inclusive and unbiased. Cosmetic stream only.
  constexpr uint32_t threshold=uint32_t(0u-100001u)%100001u;
  do {state=1664525u*state+1013904223u;} while(state<threshold);
  return state%100001u;
 }
};
struct Speech {
 uint64_t speechCount=0,remaining=0;
 unsigned speechCode=0;
 void clearSpeech(){speechCode=0;remaining=0;}
 void tick(unsigned micros){if(micros>=remaining)clearSpeech();else remaining-=micros;}
};
inline Random cosmetic;
template<class Speech> bool consider(const Context& context,unsigned roll,Speech& speech){
 if(!context.eligible()||roll!=0||speech.remaining)return false;
 speech.speechCode=4;speech.speechCount=0;speech.remaining=5000000;return true;
}
template<class Speech> bool step(Random& random,const Context& context,Speech& speech){
 return context.eligible()&&consider(context,random.roll(),speech);
}
}
