#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace EditorInput {
constexpr unsigned Select=0x1,Start=0x8,Up=0x10,Right=0x20,Down=0x40,Left=0x80,L=0x100,R=0x200,Triangle=0x1000,Circle=0x2000,Cross=0x4000,Square=0x8000;
struct Actions {
 bool primary=false,secondary=false,middle=false,shift=false,control=false,alt=false,lockX=false,lockY=false;
 bool primaryDown=false,primaryUp=false,secondaryDown=false,toggleMode=false,copy=false,paste=false,undo=false,redo=false,remove=false,back=false,pause=false,skip=false;
 int wheel=0;
};
struct PointerSample {double x,y,previousX,previousY;};
// Captured PC mouse input is relative and continues at the viewport edge.
// Warp-to-center is presentation, never additional movement for telekinesis.
inline PointerSample movePointer(double x,double y,double dx,double dy,bool captured) {
 if(captured)return {384,256,384-dx,256-dy};
 return {std::clamp(x+dx,0.,767.),std::clamp(y+dy,0.,511.),x,y};
}
struct Decoder {
 bool chord=false;
 Actions step(unsigned held,unsigned pressed,unsigned released) {
  Actions a;
  const bool select=held&Select;
  if(pressed&Select)chord=false;
  a.alt=(held&L)&&(held&R);a.shift=(held&L)&&!a.alt;a.control=(held&R)&&!a.alt;
  a.middle=held&Triangle;
  if(select) {
   a.skip=held&Square;
   a.wheel=int(bool(pressed&Up))-int(bool(pressed&Down));
   a.lockX=held&Left;a.lockY=held&Right;
   a.copy=pressed&Square;a.paste=pressed&Cross;a.undo=pressed&Triangle;
   a.redo=pressed&Start;a.remove=pressed&Circle;
   if(pressed&~Select)chord=true;
  } else {
   a.primary=held&Cross;a.secondary=held&Square;
   a.primaryDown=pressed&Cross;a.primaryUp=released&Cross;a.secondaryDown=pressed&Square;
   a.back=pressed&Circle;a.pause=pressed&Start;
  }
  if(released&Select){a.toggleMode=!chord;chord=false;}
  return a;
 }
};
}
