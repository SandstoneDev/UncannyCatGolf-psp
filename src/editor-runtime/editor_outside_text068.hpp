#pragma once
#include <algorithm>
#include <cmath>
namespace EditorOutsideText068 {
// Keep each authored face and every glyph; enlarge only the final placement.
// All coordinates remain original 768x512 units at the existing 17/32 view.
inline float scale(unsigned authoredSize){return authoredSize&&authoredSize<32?32.f/authoredSize:1.f;}
inline float offset(float full,float window,double seconds){
    if(!(full>window)||!(window>0))return 0;
    const double distance=full-window,travel=distance/24.,period=4+2*travel;
    double at=std::fmod(std::max(0.,seconds),period);
    if(at<2)return 0;
    at-=2;
    if(at<travel)return float(at*24);
    at-=travel;
    if(at<2)return float(distance);
    at-=2;
    return float(std::max(0.,distance-at*24));
}
}
