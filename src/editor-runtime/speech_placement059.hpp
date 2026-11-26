#pragma once
#include <algorithm>
#include <cmath>
namespace SpeechPlacement059 {
constexpr float FontScale=1.25f,FieldLeft=172,FieldRight=444,FieldTop=0,FieldBottom=272;
struct Point{float x,y;};
struct Placement{
 float scale=FontScale,left=0,right=0,top=0,bottom=0,anchorX=0,anchorY=0;
 Point tail[3]{};bool valid=false,below=false;
};
inline Placement place(float x,float y,unsigned lines,unsigned widest=0){
 Placement p;
 if(!lines||lines>8||!std::isfinite(x)||!std::isfinite(y))return p;
 const float content=float(18*lines-6*(lines-1));
 const float width=std::min(FieldRight-FieldLeft,std::max(128.f,float(widest)+12)*FontScale);
 const float height=std::min(FieldBottom-FieldTop,(content+12)*FontScale);
 x=std::clamp(x,FieldLeft,FieldRight);y=std::clamp(y,FieldTop,FieldBottom);
 p.anchorX=std::clamp(x,FieldLeft+width*.5f,FieldRight-width*.5f);
 p.left=p.anchorX-width*.5f;p.right=p.left+width;
 p.below=y<height+22;
 p.top=std::clamp(p.below?y+22:y-22-height,FieldTop,FieldBottom-height);p.bottom=p.top+height;
 p.anchorY=p.top+(118-(90-content)*.5f+6)*FontScale;
 const float baseX=std::clamp(x,p.left+6,p.right-6),baseY=p.below?p.top:p.bottom;
 p.tail[0]={baseX+(p.below?5:-5),baseY};
 p.tail[1]={x,std::clamp(y+(p.below?12:-12),FieldTop,FieldBottom)};
 p.tail[2]={baseX+(p.below?-5:5),baseY};p.valid=true;return p;
}
}
