#pragma once
#include <algorithm>
namespace EditorPropertyLayout067 {
struct Rect {float x,y,w,h;bool contains(float a,float b)const{return a>=x&&b>=y&&a<x+w&&b<y+h;}};
inline constexpr unsigned Rows=2;inline constexpr int FocusBase=10000,ColorFocus=11000,PreviousFocus=12000,NextFocus=12001;
inline constexpr float Top=240,Pitch=88,LabelScale=1.75f,LabelWidth=204;
inline constexpr Rect Area{24,240,208,200},Color{200,444,22,22},Previous{24,422,32,18},Next{200,422,32,18};
inline unsigned lastScroll(unsigned count){return count>Rows?count-Rows:0;}
inline unsigned clamp(unsigned scroll,unsigned count){return std::min(scroll,lastScroll(count));}
inline bool visible(unsigned index,unsigned scroll,unsigned count){scroll=clamp(scroll,count);return index<count&&index>=scroll&&index-scroll<Rows;}
inline unsigned ensureVisible(unsigned index,unsigned scroll,unsigned count){if(!count)return 0;scroll=clamp(scroll,count);index=std::min(index,count-1);if(index<scroll)scroll=index;if(index>=scroll+Rows)scroll=index-Rows+1;return scroll;}
inline unsigned shift(unsigned scroll,unsigned count,int amount){return unsigned(std::clamp(int(clamp(scroll,count))-amount,0,int(lastScroll(count))));}
inline float rowY(unsigned index,unsigned scroll){return Top+(int(index)-int(scroll))*Pitch;}
inline float labelBaseline(unsigned index,unsigned scroll){return rowY(index,scroll)+20;}
inline Rect field(unsigned index,unsigned scroll,bool boolean=false){return {boolean?212.f:24.f,rowY(index,scroll)+50,boolean?20.f:208.f,36};}
inline Rect hit(unsigned index,unsigned scroll){return field(index,scroll);}
inline int hitIndex(float x,float y,unsigned scroll,unsigned count){for(unsigned i=scroll;i<count&&i<scroll+Rows;++i)if(hit(i,scroll).contains(x,y))return int(i);return -1;}
}
