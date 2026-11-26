#pragma once
#include <algorithm>
namespace EditorWidgetReadable066 {
// Preserve the original face/strings and control hitboxes; grow ink only to fit.
inline float scale(float nominal,float width,float height,float textWidth,float ascent,float descent,float target=22.f){
 const float wanted=std::min(4.f,target/std::max(1.f,nominal));
 const float fitWidth=textWidth>0?width/textWidth:wanted;
 const float fitHeight=height/std::max(1.f,ascent+descent);
 return std::max(1.f,std::min(wanted,std::min(fitWidth,fitHeight)));
}
}
