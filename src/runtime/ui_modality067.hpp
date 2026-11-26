#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
namespace UiModality067 {
constexpr unsigned Up=0x10,Right=0x20,Down=0x40,Left=0x80,Directions=Up|Right|Down|Left,Select=1;
struct State {
 bool cursor=false;
 bool observe(bool analog,unsigned held,bool precise=false){
  const bool before=cursor;
  if(precise){if(analog)cursor=true;}
  else if((held&Directions)&&!(held&Select))cursor=false;else if(analog)cursor=true;
  return before!=cursor;
 }
};
inline bool analog(unsigned x,unsigned y){return std::abs(int(x)-128)>20||std::abs(int(y)-128)>20;}
struct Repeat {
 unsigned previous=0;uint64_t next=0;
 unsigned step(unsigned held,unsigned pressed,uint64_t now){
  const unsigned directions=(held&Select)?0:held&Directions;
  if(!directions){previous=0;next=0;return 0;}
  if(directions!=previous||(pressed&directions)){previous=directions;next=now+350000;return directions;}
  if(now>=next){next=now+90000;return directions;}return 0;
 }
};
struct Rect {float x,y,w,h;bool contains(float a,float b)const{return a>=x&&b>=y&&a<x+w&&b<y+h;}};
struct Target {int id;Rect rect;bool enabled=true;};
struct Focus {
 int id=-1;unsigned context=~0u;
 template<class Targets>int step(const Targets&items,unsigned ctx,unsigned pressed){
  if(ctx!=context){context=ctx;id=-1;}
  const Target*current=nullptr,*first=nullptr;
  for(const auto&t:items)if(t.enabled){if(!first)first=&t;if(t.id==id)current=&t;}
  if(!current){id=first?first->id:-1;return id;}
  const int dx=bool(pressed&Right)-bool(pressed&Left),dy=bool(pressed&Down)-bool(pressed&Up);
  if(!dx&&!dy)return id;
  const float x=current->rect.x+current->rect.w*.5f,y=current->rect.y+current->rect.h*.5f;
  const Target*best=nullptr;float score=std::numeric_limits<float>::max();
  for(const auto&t:items)if(t.enabled&&t.id!=id){const float a=t.rect.x+t.rect.w*.5f-x,b=t.rect.y+t.rect.h*.5f-y;
   const float primary=dx?a*dx:b*dy,secondary=dx?b:a;if(primary<=.01f)continue;
   const float cost=primary*primary+4*secondary*secondary;
   if(cost<score||(cost==score&&best&&t.id<best->id)){best=&t;score=cost;}}
  if(best)id=best->id;
  return id;
 }
 template<class Targets>const Target* selected(const Targets&items)const{for(const auto&t:items)if(t.enabled&&t.id==id)return &t;return nullptr;}
};
}
