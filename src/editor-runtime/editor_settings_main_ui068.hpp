#pragma once
#include "asset_file.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <string>
// The same locale atlas, settings skins and Theme067 slider/scroll component
// used by Main's accepted drawSettings(). All positions below are native pixels.
namespace EditorSettingsMainUi068 {
inline EditorPlayHud::Buffer widgets;
inline bool ready=false;
inline auto& glyphAdvance=EditorPlayHud::advance;
inline void patchScaled(const Patch&p,float x,float y,float sx,float sy,uint32_t tint=White){
 auto*v=static_cast<TextureVertex*>(sceGuGetMemory(2*sizeof(TextureVertex)));
 v[0]={float(p.u),float(p.v),tint,x,y,0};v[1]={float(p.u+p.width),float(p.v+p.height),tint,x+p.width*sx,y+p.height*sy,0};
 sceGuDrawArray(GU_SPRITES,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,2,nullptr,v);
}
#include "theme067.hpp"
#include "settings_readable_text068.hpp"
inline void unload(){widgets.clear();ready=false;}
inline bool load(const char*exe){
 if(ready)return true;
 constexpr unsigned bytes=256*176*4,padding=256*(256-176)*4;
 if(!widgets.allocate(bytes))return false;
 auto input=openAssetFile(companion(exe,"data/interface/settings-widgets.bin").c_str());
 if(!input||std::fread(widgets.data,bytes,1,input.get())!=1||std::fseek(input.get(),padding,SEEK_CUR)||std::fgetc(input.get())!=EOF||std::ferror(input.get())){unload();return false;}
 sceKernelDcacheWritebackRange(widgets.data,bytes);ready=true;return true;
}
inline unsigned width(const char*text){unsigned n=0;while(*text)n+=glyphAdvance[nextGlyph(text)];return n;}
inline void text(const char*value,float x,float y,float scale=.94f,uint32_t tint=White,bool center=false,unsigned maximum=0){
 const float requested=scale;const unsigned w=width(value);
 if(maximum&&w)scale=std::min(scale,float(maximum)/w);
 if(center)x-=w*scale*.5f;
 setTexture(GU_PSM_8888,512,128,EditorPlayHud::locale.data);
 while(*value){const unsigned glyph=nextGlyph(value);patchScaled({int16_t(glyph%32*16),int16_t(glyph/32*20),16,20,0,0},ViewX+x,y,scale,requested,tint);x+=glyphAdvance[glyph]*scale;}
}
inline void skin(unsigned v,float x,float y,unsigned w=130,unsigned h=23){
 setTexture(GU_PSM_8888,256,256,widgets.data);
 if(w==64&&h==23){patchScaled({0,int16_t(v),4,23,0,0},ViewX+x,y,1,1);patchScaled({4,int16_t(v),122,23,0,0},ViewX+x+4,y,56.f/122,1);patchScaled({126,int16_t(v),4,23,0,0},ViewX+x+60,y,1,1);}
 else patchScaled({0,int16_t(v),int16_t(w),int16_t(h),0,0},ViewX+x,y,1,1);
}
inline const char* diagnostic=nullptr;
inline void description(unsigned item,double seconds){
 static constexpr const char* strings[]={
  "Choose whether I get all of your attention or only most of it.",
  "Got a slow mouse cursor? Mouse moves too fast? Adjust this to whatever feels best for you!",
  "Inverts the direction you pull the mouse for a shot. Pull FORWARD for power.",
  "Allows you to buffer a shot while still moving. Release within 12 frames of coming to a stop to shoot as soon as possible.",
  "Speakers spared? Or maybe you want your speakers DESTROYED!",
  "You like my tunes? You simply MUST keep this slider all the way up. Hey wait what are you doing?",
  "Do you want to be yelled at just a little or shall the screams rupture your eardrums?",
  "...","..."};
 static std::string previous;static double since=0;
 item=std::min(item,8u);const char* message=diagnostic?diagnostic:strings[item];if(previous!=message){previous=message;since=seconds;}
 unsigned count=0;ReadableText::lines(message,unsigned(246/.74f),[&](const ReadableText::Line&){++count;});
 const unsigned pages=(count+1)/2,page=pages>1?unsigned(std::max(0.,seconds-since)/4)%pages:0;
 unsigned index=0;ReadableText::lines(message,unsigned(246/.74f),[&](const ReadableText::Line&line){
  if(index/2==page){const std::string value(line.first,line.last);text(value.c_str(),272,201+(index%2)*17,.74f,White,true,246);}++index;
 });
}
inline bool draw(const char*exe,const EditorSettingsArt::Values&v,int hover,int pressed,bool dirty,double seconds){
 if(!load(exe))return false;
 using namespace EditorSettingsArt;
 text("SETTINGS",272,16,1.25f,White,true,246);
 static constexpr const char* labels[]={"Fullscreen:","Shot Sensitivity:","Invert Shot","Buffer Shot","Master Volume:","Music Volume:","SFX Volume:"};
 for(unsigned row=0;row<6&&firstRow068+row<7;++row){
  const unsigned i=firstRow068+row;const float y=52+25*row;const bool focus=hover==int(i);
  text(labels[i],152,y+5,.74f,i?White:0xffabababu,false,108);
  skin(focus?24:0,263,y);
  if(i==Fullscreen||i==Invert||i==Buffer){
   const bool checked=i==Fullscreen?true:i==Invert?v.invert:v.buffer;
   setTexture(GU_PSM_8888,256,256,widgets.data);
   patchScaled({int16_t(checked?168:148),int16_t(focus?20:0),18,18,0,0},ViewX+319,y+3,1,1,i?White:0xffabababu);
  }else{
   const double amount=i==Sensitivity?v.sensitivity:i==Master?v.master:i==Music?v.music:v.sound;
   char value[24];std::snprintf(value,sizeof value,"%u%%",unsigned(std::round(amount*100)));
   text(value,329,y+2,.94f,White,true,99);
   text("←",267,y+3,.86f);text("→",381,y+3,.86f);
   Theme067::slider(284,y+17,87,float(i==Sensitivity?amount*.5:amount),focus);
  }
 }
 Theme067::scroll(405,53,147,float(firstRow068),6.f/7);
 skin(hover==Save?74:48,204,240,136,25);
 text("SAVE THESE SETTINGS",272,244,.82f,dirty?White:0xffabababu,true,124);
 EditorSettingsArt::drawBack(exe,false,seconds,8);
 description(hover>=0?unsigned(hover):unsigned(Save),seconds);
 (void)pressed;return true;
}
}
