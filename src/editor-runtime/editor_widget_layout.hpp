#pragma once
#include "editor_widget_text_data.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <string>
namespace EditorWidgetText {
struct Layout {float x,baseline,left,top,right,bottom;uint32_t tint;};
inline std::string format(const Spec&s,const char*value,bool affixes=true){std::string text=value?value:"";if(s.kind!=2||!affixes)return text;char*end=nullptr;const double number=std::strtod(text.c_str(),&end);if(end==text.c_str()||!end||*end)return text;char numeric[128];std::snprintf(numeric,sizeof numeric,"%.*f",int(s.decimals),number);text=numeric;if(*s.prefix)text=std::string(s.prefix)+" "+text;if(*s.suffix)text+=" "+std::string(s.suffix);return text;}
inline Layout layout(const Spec&s,float x,float y,float width,float ascent,float descent,bool hover=false,bool pressed=false,bool placeholder=false){
 const unsigned state=s.kind==1||s.kind==2?0:pressed?2:hover?1:0;const auto&m=s.margin[state];
 const float bx=x+s.rect[0],by=y+s.rect[1],w=s.rect[2],h=s.rect[3];
 const float left=m[0]+s.reserved[0],right=w-m[2]-s.reserved[1];float tx=left;
 if(s.kind==0){
  const float available=std::max(1.f,std::ceil(right-left));
  if(width<=available){if(s.alignment==1)tx+=std::floor((available-width)/2);else if(s.alignment==2)tx+=available-width;}
 }else{
  if(s.alignment==1)tx=std::max(left,float(int(w-width)/2));else if(s.alignment==2)tx=std::max(left,float(int(w-std::ceil(m[2]+width))));
  if(s.kind==2)tx-=std::max(0.f,width-(right-left));
 }
 const float baseline=by+(s.kind==0?m[1]+(h-m[1]-m[3]-ascent-descent)/2:float(int(m[1]+(h-m[1]-m[3]-ascent-descent)/2)))+ascent;
 return {bx+tx,baseline,bx+left,by,bx+right,by+h,placeholder?s.color[3]:s.color[state]};
}
}
