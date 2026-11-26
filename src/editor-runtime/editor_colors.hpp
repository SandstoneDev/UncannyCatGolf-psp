#pragma once
#include "ucg_editor_model.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <string>
namespace EditorColors {
using Color=ucg::editor::Color;
struct Hsv {double h,s,v,a;};
inline Hsv toHsv(Color c){
 const double high=std::max({c.r,c.g,c.b}),low=std::min({c.r,c.g,c.b}),d=high-low;
 double h=0;if(d>0){if(high==c.r)h=60*std::fmod((c.g-c.b)/d,6);else if(high==c.g)h=60*((c.b-c.r)/d+2);else h=60*((c.r-c.g)/d+4);}
 if(h<0)h+=360;
 return {h,high?d/high:0,high,c.a};
}
inline Color fromHsv(Hsv c){
 c.h=std::fmod(c.h,360.);if(c.h<0)c.h+=360;
 const double chroma=c.v*c.s,x=chroma*(1-std::abs(std::fmod(c.h/60,2.)-1)),m=c.v-chroma;
 Color out{0,0,0,c.a};
 if(c.h<60){out.r=chroma;out.g=x;}else if(c.h<120){out.r=x;out.g=chroma;}
 else if(c.h<180){out.g=chroma;out.b=x;}else if(c.h<240){out.g=x;out.b=chroma;}
 else if(c.h<300){out.r=x;out.b=chroma;}else{out.r=chroma;out.b=x;}
 out.r+=m;out.g+=m;out.b+=m;return out;
}
inline bool parse(std::string s,Color& out){
 if(!s.empty()&&s[0]=='#')s.erase(0,1);
 if(s.size()!=6&&s.size()!=8)return false;
 unsigned value=0;for(char c:s){unsigned v=c>='0'&&c<='9'?unsigned(c-'0'):c>='a'&&c<='f'?unsigned(c-'a'+10):c>='A'&&c<='F'?unsigned(c-'A'+10):16;if(v>15)return false;value=(value<<4)|v;}
 if(s.size()==6)value=(value<<8)|255;
 out={((value>>24)&255)/255.,((value>>16)&255)/255.,((value>>8)&255)/255.,(value&255)/255.};return true;
}
inline std::string html(Color c){char text[9];auto b=[](double d){return unsigned(std::round(std::clamp(d,0.,1.)*255));};std::snprintf(text,sizeof text,"%02x%02x%02x%02x",b(c.r),b(c.g),b(c.b),b(c.a));return text;}
inline uint32_t rgba(Color c){auto b=[](double d){return uint32_t(std::round(std::clamp(d,0.,1.)*255));};return b(c.r)|(b(c.g)<<8)|(b(c.b)<<16)|(b(c.a)<<24);}
}
