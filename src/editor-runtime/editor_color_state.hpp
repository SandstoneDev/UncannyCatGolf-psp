#pragma once
#include "editor_colors.hpp"
#include "editor_ok_color.hpp"
#include "editor_named_colors.hpp"
namespace EditorColors {
// Godot 4.4 ColorModeRGB/HSV/RAW channel units. Linear mode exposes the
// original floating-point components; it does not gamma-convert the record.
struct State {
 enum class Mode {RGB,HSV,Linear,OKHSL};Mode mode=Mode::RGB;Color value{};
 double cachedHue=0,cachedSaturation=0;
 double cachedOkHue=0,cachedOkSaturation=0;
 ok_color::HSL ok()const{auto c=ok_color::srgb_to_okhsl({float(value.r),float(value.g),float(value.b)});if(!std::isfinite(c.h))c.h=0;if(!std::isfinite(c.s))c.s=0;if(!std::isfinite(c.l))c.l=0;return c;}
 void set(Color c){value=c;auto hsv=toHsv(c);if(hsv.s>0)cachedHue=hsv.h;if(hsv.v>0)cachedSaturation=hsv.s;}
 double rawChannel(unsigned i)const{if(i>=4)return 0;if(i==3)return value.a*(mode==Mode::Linear?1:255);if(mode==Mode::OKHSL){auto c=ok();return i==0?(c.s>0?c.h*360:cachedOkHue):i==1?(c.l>0?c.s:cachedOkSaturation)*100:c.l*100;}if(mode==Mode::HSV){auto c=toHsv(value);return i==0?(c.s>0?c.h:cachedHue):i==1?(c.v>0?c.s:cachedSaturation)*100:c.v*100;}const double c[]={value.r,value.g,value.b};return c[i]*(mode==Mode::RGB?255:1);}
 double maximum(unsigned i)const{if(i>=4)return 0;if(i==3)return mode==Mode::Linear?1:255;if(mode==Mode::HSV||mode==Mode::OKHSL)return i==0?359:100;if(mode==Mode::Linear)return 100;double high=255;while(high<rawChannel(i)&&high<1073741823)high=(high+1)*2-1;return high;}
 double step()const{return mode==Mode::Linear?double(float(.001)):1;}
 double quantized(unsigned i,double number)const{return std::clamp(std::floor(number/step()+.5)*step(),0.,maximum(i));}
 double channel(unsigned i)const{return i<4?quantized(i,rawChannel(i)):0;}
 std::string channelText(unsigned i)const{char text[96];std::snprintf(text,sizeof text,mode==Mode::Linear?"%.3f":"%.0f",channel(i));return text;}
 bool edit(unsigned i,double number){
  if(i>=4||!std::isfinite(number))return false;
  double channels[]={channel(0),channel(1),channel(2),channel(3)};const double changed=quantized(i,number);
  // Range emits no value_changed signal when the displayed value is unchanged.
  if(changed==channels[i])return true;
  channels[i]=changed;
  const float alpha=float(channels[3]/(mode==Mode::Linear?1.:255.));
  if(mode==Mode::HSV){
   const float h=float(channels[0]/360),sat=float(channels[1]/100),v=float(channels[2]/100);const float at=(h-std::floor(h))*6;const unsigned sector=unsigned(at);const float f=at-sector,p=v*(1-sat),q=v*(1-sat*f),t=v*(1-sat*(1-f));
   switch(sector%6){case 0:value={v,t,p,alpha};break;case 1:value={q,v,p,alpha};break;case 2:value={p,v,t,alpha};break;case 3:value={p,q,v,alpha};break;case 4:value={t,p,v,alpha};break;default:value={v,p,q,alpha};}
   cachedHue=channels[0];cachedSaturation=channels[1]/100;
  }else if(mode==Mode::OKHSL){
   auto c=ok_color::HSL{float(channels[0]/360),float(channels[1]/100),float(channels[2]/100)};auto rgb=ok_color::okhsl_to_srgb(c);value={rgb.r,rgb.g,rgb.b,alpha};cachedOkHue=channels[0];cachedOkSaturation=channels[1]/100;
  }else{
   const double scale=mode==Mode::RGB?255.:1.;value={float(channels[0]/scale),float(channels[1]/scale),float(channels[2]/scale),alpha};auto c=toHsv(value);if(c.s>0)cachedHue=c.h;if(c.v>0)cachedSaturation=c.s;
  }
  return true;
 }
 bool hex(std::string text){uint32_t named=0;if(EditorNamedColors::find(text,named)){set({float((named>>24)&255)/255.f,float((named>>16)&255)/255.f,float((named>>8)&255)/255.f,float(named&255)/255.f});return true;}if(!text.empty()&&text[0]=='#')text.erase(0,1);if(text.size()==3||text.size()==4){std::string full;for(char c:text){full+=c;full+=c;}text=full;}Color c;if(!parse(text,c))return false;set(c);return true;}
};
}
