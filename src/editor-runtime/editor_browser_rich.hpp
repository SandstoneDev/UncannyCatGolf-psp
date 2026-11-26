#pragma once
// Source effect equations: Godot4.4.1 scene/gui/rich_text_label.cpp.
// Font shaping shares the independently verified original FontFile metrics.
namespace EditorBrowserRich {
namespace Text=EditorPlayHud::FontText;
struct Style {unsigned font=8;uint32_t color=White;bool wave=false,shake=false,rainbow=false;float amp=20,freq=5,level=5,rate=20,saturation=.8f,value=.8f,rainbowFreq=1,speed=1;};
struct Parsed{std::vector<Text::Character> text;std::vector<Style> styles;};
inline float option(const std::string&tag,const char*key,float fallback){
 const std::string needle=std::string(key)+"=";const auto at=tag.find(needle);if(at==std::string::npos)return fallback;char*end=nullptr;const float value=std::strtof(tag.c_str()+at+needle.size(),&end);return end&&end!=tag.c_str()+at+needle.size()&&std::isfinite(value)?value:fallback;
}
inline Parsed parse(const std::string&input){
 Parsed out;Style style;std::vector<std::pair<std::string,Style>> stack;
 for(size_t i=0;i<input.size();){
  if(input[i]=='['){const auto end=input.find(']',i+1);if(end!=std::string::npos){const std::string tag=input.substr(i+1,end-i-1);std::string name=tag.substr(0,tag.find_first_of(" ="));const bool close=!name.empty()&&name[0]=='/';if(close)name.erase(0,1);
   if(name=="wave"||name=="shake"||name=="rainbow"||name=="font"||name=="color"){
    if(close){for(size_t n=stack.size();n;--n)if(stack[n-1].first==name){style=stack[n-1].second;stack.resize(n-1);break;}}
    else {stack.emplace_back(name,style);
     if(name=="wave"){style.wave=true;style.amp=option(tag,"amp",20);style.freq=option(tag,"freq",5);}
     else if(name=="shake"){style.shake=true;style.level=option(tag,"level",5);style.rate=std::max(.001f,option(tag,"rate",20));}
     else if(name=="rainbow"){style.rainbow=true;style.saturation=option(tag,"sat",.8f);style.value=option(tag,"val",.8f);style.rainbowFreq=std::max(0.f,option(tag,"freq",1));style.speed=option(tag,"speed",1);}
     else if(name=="font"&&tag.find("impact.ttf")!=std::string::npos)style.font=10;
     else if(name=="color"){const auto hash=tag.find('#');if(hash!=std::string::npos){unsigned rgb=0;if(std::sscanf(tag.c_str()+hash+1,"%6x",&rgb)==1)style.color=0xff000000u|((rgb>>16)&255)|(rgb&0xff00)|((rgb&255)<<16);}}
    }
    i=end+1;continue;
   }
  }}
  const char*begin=input.c_str()+i;const char*at=begin;const unsigned code=EditorArt::codepoint(at);if(at==begin)break;i+=size_t(at-begin);out.text.push_back({code,style.font});out.styles.push_back(style);
 }return out;
}
inline uint32_t random(unsigned index,unsigned tick){uint32_t x=index*0x9e3779b9u+tick*0x85ebca6bu+0x13ab21u;x^=x>>16;x*=0x7feb352du;x^=x>>15;x*=0x846ca68bu;return x^(x>>16);}
inline float draw(const std::string&input,float x,float y,float width,float height,float offset,float clock){
 const auto parsed=parse(input);const auto layout=Text::lines(parsed.text,width,true);float fullHeight=0;for(const auto&line:layout){const auto m=Text::metrics(parsed.text,line.first,line.last);fullHeight+=m[0]+m[1];}if(offset==0&&fullHeight>height)offset=EditorOutsideText068::offset(fullHeight,height,clock);float top=y-offset;
 const auto oldClip=EditorRender::clip;const auto ui=EditorArt::uiScissor;EditorRender::clip={float(std::max(ViewX,ui[0])),float(std::max(0,ui[1])),float(std::min(ViewX+408,ui[2])),float(std::min(272,ui[3]))};
 for(const auto&line:layout){const auto metrics=Text::metrics(parsed.text,line.first,line.last);const float baseline=top+metrics[0];for(unsigned layer=0;layer<3;++layer){unsigned index=line.first;
  Text::shape(parsed.text,line.first,line.last,[&](Text::Character c,const Text::Glyph&,float pen,float){while(index<line.last&&!Text::glyph(parsed.text[index].font,parsed.text[index].code))++index;if(index>=line.last)return;const auto style=parsed.styles[index++];float dx=0,dy=0;uint32_t tint=style.color;
   if(style.wave)dy+=std::sin(style.freq*clock+(x+pen)/50)*(style.amp/10);
   if(style.shake){const float phase=clock*style.rate*2;const unsigned tick=unsigned(std::max(0.f,phase));const float blend=phase-tick;const float a=(random(index,tick-1)%2147483647u)*(6.28318530718f/2147483647.f),b=(random(index,tick)%2147483647u)*(6.28318530718f/2147483647.f);dx+=(std::sin(a)+(std::sin(b)-std::sin(a))*blend)*style.level/10;dy+=(std::cos(a)+(std::cos(b)-std::cos(a))*blend)*style.level/10;}
   if(style.rainbow){tint=Text::hsv(style.rainbowFreq*std::abs(clock*style.speed+(x+pen)/50),style.saturation);const auto channel=[&](unsigned shift){return unsigned(std::clamp(float((tint>>shift)&255)*style.value,0.f,255.f)+.5f);};tint=0xff000000u|channel(0)|(channel(8)<<8)|(channel(16)<<16);}
   if(baseline+dy>=y-32&&baseline+dy<=y+height+32)Text::drawGlyph(c.font,c.code,x+pen+dx,baseline+dy,tint,layer,pen+dx,baseline+dy-y);
  });}top+=metrics[0]+metrics[1];
 }
 EditorRender::clip=oldClip;EditorArt::scissor(ui[0],ui[1],ui[2],ui[3]);return top+offset-y;
}
}
