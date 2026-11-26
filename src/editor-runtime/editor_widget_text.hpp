#pragma once
#include "editor_widget_layout.hpp"
#include "editor_legacy_text.hpp"
#include "editor_widget_readable066.hpp"
#include "editor_toolbar_layout066.hpp"
#include "editor_text_layout068.hpp"
namespace EditorWidgetText {
inline bool draw(unsigned id,float x,float y,const char*value,bool hover,bool pressed,bool placeholder){
 if(id>=std::size(Specs)||id>=std::size(EditorArt::Packed::Controls))return false;
 Spec readable=Specs[id];
 const auto&control=EditorArt::Packed::Controls[id];const auto bounds=EditorTextLayout068::rect(id,EditorToolbarLayout066::rect(id,{x,y,control.w,control.h}));readable.rect[2]=bounds.w-(control.w-readable.rect[2]);readable.rect[3]=bounds.h;
 if(EditorToolbarLayout066::toolbar(id)){const auto r=EditorToolbarLayout066::rect(id,{});readable.rect[2]=r.w;readable.rect[3]=r.h;}
 if(EditorToolbarLayout066::toolbar(id))for(auto&margin:readable.margin){margin[0]=margin[2]=2.f;}
 if(readable.kind<=2)for(auto&margin:readable.margin){margin[0]=std::min(margin[0],8.f);margin[1]=std::min(margin[1],readable.kind==0?2.f:0.f);margin[2]=std::min(margin[2],8.f);margin[3]=std::min(margin[3],readable.kind==0?2.f:0.f);}
 const auto&c=EditorArt::Packed::Controls[id];const unsigned face=c.font==0?3:c.font;
 if(readable.kind==2&&std::strspn(readable.prefix," \t")==std::strlen(readable.prefix))readable.prefix="";
 if(id>=5&&id<=8)readable.reserved[0]=28;
 const auto&s=readable;
 if(s.kind==3)return false;
 const bool oldOutline=EditorLegacyText::outline;EditorLegacyText::outline=s.kind==0;struct RestoreOutline{bool v;~RestoreOutline(){EditorLegacyText::outline=v;}}restoreOutline{oldOutline};
 const bool affixes=true;
 const std::string text=format(s,value,affixes);const auto chars=EditorLegacyText::characters(face,text.c_str());const auto&font=EditorRender::LegacyFonts::Fonts[face];float ascent=font.ascent,descent=font.descent;
 for(unsigned code:chars)if(const auto*g=EditorLegacyText::glyph(face,code)){ascent=std::max(ascent,g->ascent);descent=std::max(descent,g->descent);}
 const float measured=EditorLegacyText::measure(face,text.c_str());float scale=1.f;
 if(s.kind<=2){
  float margins[4]={};for(const auto&state:s.margin)for(unsigned i=0;i<4;++i)margins[i]=std::max(margins[i],state[i]);
  const float height=std::max(1.f,s.rect[3]-margins[1]-margins[3]);
  scale=EditorWidgetReadable066::scale(font.size,1e30f,height,measured,ascent,descent,EditorToolbarLayout066::toolbar(id)?18.f:((id==26||id==44)?20.f:24.f));
 }
 auto p=layout(s,x,y,measured*scale,ascent*scale,descent*scale,hover,pressed,placeholder);
 if(s.kind<=2&&measured*scale>p.right-p.left)p.x=p.left-EditorTextFlow068::offset(measured*scale,p.right-p.left);
 const auto old=EditorArt::uiScissor;
 const int left=std::max(old[0],ViewX+int(std::ceil(p.left*EditorArt::Scale))),top=std::max(old[1],int(std::ceil(p.top*EditorArt::Scale))),right=std::min(old[2],ViewX+int(std::floor(p.right*EditorArt::Scale))),bottom=std::min(old[3],int(std::floor(p.bottom*EditorArt::Scale)));
 if(left<right&&top<bottom){EditorArt::scissor(left,top,right,bottom);EditorLegacyText::drawScaled(face,text.c_str(),p.x,p.baseline,p.tint,1e30f,scale);EditorArt::scissor(old[0],old[1],old[2],old[3]);}return true;
}
inline void install(){EditorArt::controlTextHook=draw;}
}
