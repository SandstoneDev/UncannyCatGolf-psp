#pragma once
#include <algorithm>
#include <cmath>
namespace EditorListLayout067 {
inline constexpr unsigned ObjectRows=2,DecoRows=4,DecoColumns=1;
inline constexpr float ObjectRow=60.f,DecoRow=60.f;
inline unsigned lastScroll(unsigned count,unsigned rows){return count>rows?count-rows:0;}
inline int hitRow(float y,float pitch,unsigned rows){
 if(!std::isfinite(y)||y<0||y>=pitch*rows)return -1;
 return int(y/pitch);
}
inline unsigned ensureVisible(unsigned selected,unsigned scroll,unsigned count,unsigned rows){
 if(!count)return 0;
 selected=std::min(selected,count-1);scroll=std::min(scroll,lastScroll(count,rows));
 if(selected<scroll)scroll=selected;
 if(selected>=scroll+rows)scroll=selected-rows+1;
 return scroll;
}
}
