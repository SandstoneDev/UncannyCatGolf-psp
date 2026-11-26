#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <new>
namespace BortSim {
constexpr uint64_t RoundUs=30000000,ExplosionUs=566667;
enum class Phase { Prompt, Playing, Congrats, GameOver };
struct Actor {
 float x=0,y=0,vx=0,vy=0,hue=0;
 unsigned wingTime=3,wingTimer=0,frame=0;
 uint64_t explosionUs=0;
 bool bashed=false,visible=true;
};
struct Run {
 Phase phase=Phase::Prompt;
 unsigned round=0,left=0,score=0,count=0,hits=0;
 uint64_t timerUs=0;
 uint32_t random=1;
 bool cleared=false,lost=false;
 std::unique_ptr<Actor[]> actors;
 uint32_t draw(){random^=random<<13;random^=random>>17;return random^=random<<5;}
 float unit(){return draw()/4294967295.f;}
 unsigned integer(unsigned low,unsigned high){return low+draw()%(high-low+1);}
 void reset(uint32_t seed) {
  actors.reset();phase=Phase::Prompt;round=left=score=count=hits=0;
  timerUs=0;random=seed?seed:1;cleared=lost=false;
 }
 unsigned seconds() const {return static_cast<unsigned>((timerUs+999999)/1000000);}
 unsigned rank() const {return std::min(6u,score/15000);}
 bool spawn() {
  if (round>=std::numeric_limits<unsigned>::max()/5) return false;
  const unsigned next=round+1,total=next*5;
  std::unique_ptr<Actor[]> fresh(new(std::nothrow) Actor[total]);
  if (!fresh) return false;
  for(unsigned i=0;i<total;++i) {
   auto&a=fresh[i];
   const float speed=integer(100+next*30,600+next*30);
   const float angle=.261f+unit()*1.048f+integer(0,3)*1.57079632679f;
   a.vx=std::cos(angle)*speed;a.vy=std::sin(angle)*speed;
   a.x=integer(320,704);a.y=integer(128,448);
   a.wingTime=integer(3,10);a.hue=unit();
  }
  actors=std::move(fresh);round=next;count=left=total;
  phase=Phase::Playing;timerUs=RoundUs;return true;
 }
 bool start(){return phase==Phase::Prompt && spawn();}
 bool finishCongrats(){return phase==Phase::Congrats && spawn();}
 static void move(Actor&a,float seconds) {
  // Cage faces are x256/768 and y64/512; the authored body half-size is
  // (26.667*.6/2,60*.533/2), with 0.08 margin recovery on nonzero motion. Contact uses analytic TOI;
  // the Godot iterative solver differs by at most0.1 authorpx in reference probes.
  constexpr float left=264.0001f,right=759.9999f,top=79.99f,bottom=496.01f;
  if(a.vx!=0 || a.vy!=0) {a.x=std::clamp(a.x,left+.08f,right-.08f);a.y=std::clamp(a.y,top+.08f,bottom-.08f);}
  const float dx=a.vx*seconds,dy=a.vy*seconds;
  float tx=2,ty=2;
  if(dx>0 && a.x+dx>right)tx=std::max(0.f,(right-a.x)/dx);
  if(dx<0 && a.x+dx<left)tx=std::max(0.f,(left-a.x)/dx);
  if(dy>0 && a.y+dy>bottom)ty=std::max(0.f,(bottom-a.y)/dy);
  if(dy<0 && a.y+dy<top)ty=std::max(0.f,(top-a.y)/dy);
  const float travel=std::min({1.f,tx,ty});
  a.x+=dx*travel;a.y+=dy*travel;
  if(tx<=1 && tx<=ty)a.vx=-a.vx;
  else if(ty<=1)a.vy=-a.vy;
 }
 void step(float px,float py,bool click,uint64_t us=16667) {
  hits=0;cleared=lost=false;
  if(phase!=Phase::Playing && phase!=Phase::Congrats)return;
  for(unsigned i=0;i<count;++i) {
   auto&a=actors[i];if(!a.visible)continue;
   const bool hover=std::abs(px-a.x)<=20 && std::abs(py-a.y)<=32;
   if(a.wingTimer>a.wingTime){a.frame^=1;a.wingTimer=0;}else ++a.wingTimer;
   move(a,us/1000000.f);
   if(a.bashed) {
    a.explosionUs+=us;
    if(a.explosionUs>=ExplosionUs)a.visible=false;
   }else if(click && hover) {
    a.bashed=true;a.vx=a.vy=0;a.explosionUs=0;
    --left;++hits;score+=50;
   }
  }
  if(phase==Phase::Playing && !left) {
   score+=seconds()*100;phase=Phase::Congrats;cleared=true;return;
  }
  if(phase==Phase::Playing) {
   if(us>=timerUs) {
    timerUs=0;phase=Phase::GameOver;lost=true;
    actors.reset();count=0;
   }else timerUs-=us;
  }
 }
};
}
