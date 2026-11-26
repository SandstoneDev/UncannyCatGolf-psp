#pragma once
#include "editor_hud_fonts.hpp"
#include "editor_font_masks.hpp"
#include "editor_outside_text068.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iterator>
#include <string>
#include <vector>
// Original font rasters and fractional engine metrics. Coordinates passed to
// this helper are in the original 768x512 viewport; only the final vertices
// apply the fixed PSP viewport scale.
namespace EditorPlayHud {
namespace FontText {
using Glyph=EditorRender::HudFonts::Glyph;
inline constexpr unsigned Name=0,Author=1,Time=2,TimeMil=3,Bonus=4,Strokes=5,TkFraction=6,Result16=7,Result12=8,Result10=9,WorldDescription=10;
// Preserve authored timer/strokes; enlarge only the smallest non-timer faces.
inline float readableScale(unsigned font){
  switch(font){
  case Author:return 12.f/7;
  case Result16:return 18.f/16;
  case Result12:case WorldDescription:return 14.f/12;
  case Result10:return 12.f/10;
  default:return 1.f;
  }
}
inline std::string strokeKey;
inline unsigned strokeTick=0;
inline double introEpoch=0;
inline void reset(){strokeKey.clear();strokeTick=0;introEpoch=shaderTime;}
struct Character {unsigned code,font;};
struct Line {unsigned first,last;float width;};
inline const Glyph* glyph(unsigned font,unsigned code){
  if(font>=std::size(EditorRender::HudFonts::Fonts))return nullptr;
  const auto&f=EditorRender::HudFonts::Fonts[font];
  for(unsigned i=0;i<f.count;++i){const auto&g=EditorRender::HudFonts::Glyphs[f.first+i];if(g.code==code)return &g;}
  return nullptr;
}
inline const EditorRender::HudFonts::Kern* pairKerning(unsigned font,unsigned left,unsigned right){
  const auto&pairs=EditorRender::HudFonts::Kerning;unsigned first=0,last=std::size(pairs);
  while(first<last){const unsigned middle=first+(last-first)/2;const auto&k=pairs[middle];if(k.font<font||(k.font==font&&(k.left<left||(k.left==left&&k.right<right))))first=middle+1;else last=middle;}
  if(first<std::size(pairs)){const auto&k=pairs[first];if(k.font==font&&k.left==left&&k.right==right)return &k;}return nullptr;
}
inline float kern(unsigned font,unsigned left,unsigned right){const auto*k=pairKerning(font,left,right);return k?k->x:0;}
inline std::vector<Character> characters(const char*text,unsigned font){
  std::vector<Character> result;
  while(*text)result.push_back({EditorArt::codepoint(text),font});
  return result;
}
template<class Visitor>inline float shape(const std::vector<Character>&text,unsigned first,unsigned last,const Visitor&visit){
  float pen=0,carry=0;unsigned previousFont=UINT32_MAX,previousFace=UINT32_MAX;
  for(unsigned i=first;i<last;++i){const auto c=text[i];const auto*g=glyph(c.font,c.code);if(!g)continue;
    if(c.font!=previousFont||g->face!=previousFace||c.code==' '||c.code=='\t'||c.code=='\n')carry=0;
    const EditorRender::HudFonts::Kern *before=nullptr,*after=nullptr;
    if(i>first){const auto prev=text[i-1];const auto*p=glyph(prev.font,prev.code);if(p&&prev.font==c.font&&p->face==g->face)before=pairKerning(c.font,prev.code,c.code);}
    if(i+1<last){const auto next=text[i+1];const auto*n=glyph(next.font,next.code);if(n&&next.font==c.font&&n->face==g->face)after=pairKerning(c.font,c.code,next.code);}
    float advance=g->advance+(before?before->db:0)+(after?after->da:0);
    const float rawOffset=(before?before->ob:0)+(after?after->oa:0);
    const float offset=g->snap?std::round(carry+rawOffset):rawOffset;
    if(g->snap){const float full=carry+advance;advance=std::round(full);carry=full-advance;}
    else carry=0;
    const float sizeScale=readableScale(c.font);visit(c,*g,pen+offset*sizeScale,advance*sizeScale);pen+=advance*sizeScale;previousFont=c.font;previousFace=g->face;
  }return pen;
}
inline float width(const std::vector<Character>&text,unsigned first,unsigned last){return shape(text,first,last,[](Character,const Glyph&,float,float){});}
inline std::array<float,2> metrics(const std::vector<Character>&text,unsigned first,unsigned last){
  std::array<float,2> result{};for(unsigned i=first;i<last;++i)if(const auto*g=glyph(text[i].font,text[i].code)){const auto&font=EditorRender::HudFonts::Fonts[text[i].font];const float sizeScale=readableScale(text[i].font);result[0]=std::max({result[0],g->ascent*sizeScale,font.ascent*sizeScale});result[1]=std::max({result[1],g->descent*sizeScale,font.descent*sizeScale});}return result;
}
inline std::vector<Line> lines(const std::vector<Character>&text,float maxWidth,bool wrap){
  std::vector<Line> out;unsigned first=0;
  while(first<text.size()){
    if(text[first].code=='\n'){out.push_back({first,first,0});++first;continue;}
    unsigned at=first,lastBreak=first;
    for(;at<text.size()&&text[at].code!='\n';++at){const auto c=text[at];
      if(wrap&&at>first&&width(text,first,at+1)>maxWidth)break;
      if(c.code==' '||c.code=='\t'||c.code=='-')lastBreak=at+1;
    }
    unsigned last=at;
    if(wrap&&at<text.size()&&text[at].code!='\n'&&lastBreak>first)last=lastBreak;
    if(last==first)last=std::min<unsigned>(first+1,text.size());
    unsigned visible=last;while(visible>first&&(text[visible-1].code==' '||text[visible-1].code=='\t'))--visible;
    out.push_back({first,visible,width(text,first,visible)});first=last;
    while(first<text.size()&&(text[first].code==' '||text[first].code=='\t'))++first;
    if(first<text.size()&&text[first].code=='\n')++first;
  }
  if(out.empty()||(!text.empty()&&text.back().code=='\n'))out.push_back({unsigned(text.size()),unsigned(text.size()),0});
  return out;
}
inline float measure(const char*text,unsigned font){const auto value=characters(text,font);float result=0;for(const auto&line:lines(value,1e30f,false))result=std::max(result,line.width);return std::ceil(result);}
inline void drawGlyph(unsigned font,unsigned code,float x,float baseline,uint32_t tint,unsigned layer,float localPen,float localBaseline){
  const auto*g=glyph(font,code);if(!g||!g->w||!g->h||!(EditorRender::FontMasks::HudFlags[font]&(1u<<layer)))return;
  const float sizeScale=readableScale(font);const float left=ViewX+(x+g->ox*sizeScale)*EditorArt::Scale,top=(baseline+g->oy*sizeScale)*EditorArt::Scale;
  const float right=left+g->w*EditorArt::Scale*sizeScale,bottom=top+g->h*EditorArt::Scale*sizeScale;
  const float u=g->u,v=g->v,ur=u+g->w,vb=v+g->h;
  const std::array<EditorRender::Vertex,4> quad{{{left,top,u,v},{right,top,ur,v},{right,bottom,ur,vb},{left,bottom,u,vb}}};
  EditorRender::FontMasks::draw(EditorRender::FontMasks::Hud,g->page,static_cast<EditorRender::FontMasks::Layer>(layer),quad,tint,nullptr,nullptr,localPen,localBaseline);
}

template<class Tint>inline void draw(const std::vector<Character>&text,float x,float y,float w,float h,bool center,bool middle,bool wrap,float spacing,const Tint&tint){
  if(text.empty())return;
  const auto layout=lines(text,w,wrap);
  float height=0;for(const auto&line:layout){const auto m=metrics(text,line.first,line.last);height+=m[0]+m[1];}height+=(layout.size()-1)*spacing;
  float top=y+(middle?float(int(h-height)/2):0);if(layout.size()>1&&h>0&&height>h)top=y-EditorOutsideText068::offset(height,h,EditorPlayHud::shaderTime);
  const auto saved=EditorRender::clip;const auto savedUi=EditorArt::uiScissor;
  EditorRender::clip={float(std::max(ViewX,savedUi[0])),float(std::max(0,savedUi[1])),float(std::min(ViewX+408,savedUi[2])),float(std::min(272,savedUi[3]))};
  for(const auto&line:layout){const auto m=metrics(text,line.first,line.last);const float baseline=top+m[0],origin=x+(center?float(int(std::max(w,std::ceil(line.width))-std::ceil(line.width))/2):0);
    // Source Label draws each complete line's shadow, then outline, then fill.
    // Adjacent outlines must not cover an already drawn neighbouring fill.
    for(unsigned layer=0;layer<3;++layer)
      shape(text,line.first,line.last,[&](Character c,const Glyph&,float pen,float){drawGlyph(c.font,c.code,origin+pen,baseline,tint(origin+pen-x,c.code),layer,pen,baseline-y);});
    top+=m[0]+m[1]+spacing;
  }
  EditorRender::clip=saved;EditorArt::scissor(savedUi[0],savedUi[1],savedUi[2],savedUi[3]);
}
inline void label(const char*text,unsigned font,float x,float y,float w,float h,bool center=false,bool middle=false,bool wrap=false,float spacing=3,uint32_t tint=White,bool clipToRect=false){
  if(h==0){const auto&f=EditorRender::HudFonts::Fonts[font];y+=f.ascent*(1-readableScale(font));h=f.height*readableScale(font);if(y+h>512)y=512-h;}
  (void)clipToRect;
  const auto previous=EditorArt::uiScissor;
  if(w>0&&h>0)EditorArt::scissor(std::max(previous[0],ViewX+int(std::floor(x*EditorArt::Scale))),std::max(previous[1],int(std::floor(y*EditorArt::Scale))),std::min(previous[2],ViewX+int(std::ceil((x+w)*EditorArt::Scale))),std::min(previous[3],int(std::ceil((y+h)*EditorArt::Scale))));
  const float fullWidth=measure(text,font);if(!wrap&&w>0&&fullWidth>w)x-=EditorOutsideText068::offset(fullWidth,w,EditorPlayHud::shaderTime);
  draw(characters(text,font),x,y,w,h,center,middle,wrap,spacing,[&](float,unsigned){return tint;});
  EditorArt::scissor(previous[0],previous[1],previous[2],previous[3]);
}
inline uint32_t hsv(double hue,double saturation){
  hue-=std::floor(hue);const double at=hue*6,f=at-std::floor(at),p=1-saturation,q=1-saturation*f,t=1-saturation*(1-f);double r=1,g=1,b=1;
  switch(unsigned(at)%6){case 0:g=t;b=p;break;case 1:r=q;b=p;break;case 2:r=p;b=t;break;case 3:r=p;g=q;break;case 4:r=t;g=p;break;default:g=p;b=q;break;}
  const auto channel=[](double v){return unsigned(std::clamp(v,0.,1.)*255+.5);};return 0xff000000u|channel(r)|(channel(g)<<8)|(channel(b)<<16);
}
} // namespace FontText
} // namespace EditorPlayHud
