#pragma once
namespace EditorToolbarLayout066 {
struct Rect {
 float x,y,w,h;
 bool contains(float px,float py)const{return px>=x&&py>=y&&px<x+w&&py<y+h;}
};
inline constexpr bool toolbar(unsigned id){return id>=47&&id<=51;}
inline constexpr Rect rect(unsigned id,Rect original){constexpr float x[]={259,337,417,493,675},w[]={76,78,74,180,93};return toolbar(id)?Rect{x[id-47],2,w[id-47],36}:original;}
inline bool blocksWorld(float x,float y){return x>=256&&x<768&&y>=0&&y<40;}
}
