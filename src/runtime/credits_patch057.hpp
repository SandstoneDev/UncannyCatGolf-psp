#pragma once
#include "asset_file.hpp"
#include "credits_composite057.hpp"
#include <string>
#include <zlib.h>
namespace CreditsPatch057 {
inline uint16_t u16(const uint8_t* p){uint16_t v;std::memcpy(&v,p,2);return v;}
inline uint32_t u32(const uint8_t* p){uint32_t v;std::memcpy(&v,p,4);return v;}
struct Frame{const uint8_t* actors=nullptr;const uint8_t* spans=nullptr;unsigned actorCount=0,spanCount=0;bool scare=false;};
inline bool validate(const uint8_t* data,unsigned bytes,Frame& frame){
 frame={};if(bytes<8)return false;const unsigned spans=u16(data),actors=u16(data+2);
 if(actors>67||spans>272*65||u32(data+4)>1||actors>(bytes-8)/32)return false;
 frame={data+8,data+8+actors*32,actors,spans,u32(data+4)!=0};
 for(unsigned i=0;i<actors;++i){CreditsComposite057::Actor a;std::memcpy(&a,frame.actors+32*i,32);if(a.kind>2)return false;const float* f=&a.a;for(unsigned j=0;j<7;++j)if(!std::isfinite(f[j])||std::abs(f[j])>100000)return false;if(a.alpha<0||a.alpha>1.001f)return false;}
 const uint8_t* p=frame.spans;unsigned previousY=0,previousEnd=0;
 for(unsigned i=0;i<spans;++i){if(unsigned(data+bytes-p)<6)return false;const unsigned y=u16(p),x=u16(p+2),n=u16(p+4)&0x7fff,front=u16(p+4)>>15;p+=6;
  if(y>=272||x>=272||!n||n>272-x||(i&&(y<previousY||(y==previousY&&x<previousEnd))))return false;
  const unsigned size=n*(front?6:2);if(size>unsigned(data+bytes-p))return false;p+=size;previousY=y;previousEnd=x+n;
 }
 return p==data+bytes;
}
class Reader{
 AssetFile file;std::string path;unsigned cached=3174,bankBytes=0,decodedLimit=0,packedLimit=0;Frame frame{};
public:
 static constexpr unsigned Frames=3174,DecodedMax=512*272*2,PackedMax=112*1024;
 bool open(const std::string& path){
  reset();this->path.clear();bankBytes=decodedLimit=packedLimit=0;auto f=openAssetFile(path.c_str());uint32_t h[6]{};
  if(!f||std::fseek(f.get(),0,SEEK_END))return false;
  const long length=std::ftell(f.get());
  if(length<long(24+Frames*12)||std::fseek(f.get(),0,SEEK_SET)||std::fread(h,24,1,f.get())!=1||h[0]!=0x31504355||h[1]!=1||h[2]!=Frames||!h[3]||h[3]>DecodedMax||!h[4]||h[4]>PackedMax||h[5])return false;
  unsigned end=24+Frames*12;for(unsigned i=0;i<Frames;++i){uint32_t r[3];if(std::fread(r,12,1,f.get())!=1||r[0]!=end||!r[1]||r[1]>h[4]||r[2]<8||r[2]>h[3]||end>unsigned(length)||r[1]>unsigned(length)-end)return false;end+=r[1];}
  if(end!=unsigned(length))return false;
  this->path=path;bankBytes=unsigned(length);decodedLimit=h[3];packedLimit=h[4];return true;
 }
 void reset(){file.reset();cached=Frames;frame={};}
 void close(){reset();path.clear();}
 const Frame* prepare(unsigned index,uint8_t* decoded,uint8_t* packed){
  if(path.empty()||index>=Frames)return nullptr;
  if(!file){file=openAssetFile(path.c_str());if(file&&(std::fseek(file.get(),0,SEEK_END)||std::ftell(file.get())!=long(bankBytes)))file.reset();}
  if(!file)return nullptr;
  // The caller may overwrite decoded with another shared workspace user, so
  // its scene lifetime is the cache lease; reset() revokes it on transitions.
  if(index==cached)return &frame;
  // Any attempted replacement revokes the old decoded-buffer lease, even if
  // read/decode/validation subsequently fails. Never return stale frame views.
  cached=Frames;frame={};
  uint32_t r[3];
  if(std::fseek(file.get(),24+12*index,SEEK_SET)||std::fread(r,12,1,file.get())!=1||
     r[0]<24+Frames*12||r[0]>bankBytes||!r[1]||r[1]>packedLimit||r[1]>PackedMax||r[1]>bankBytes-r[0]||r[2]<8||r[2]>decodedLimit||r[2]>DecodedMax||
     std::fseek(file.get(),r[0],SEEK_SET)||std::fread(packed,r[1],1,file.get())!=1)return nullptr;
  Frame next{};unsigned long size=r[2],consumed=r[1];if(uncompress2(decoded,&size,packed,&consumed)!=Z_OK||size!=r[2]||consumed!=r[1]||!validate(decoded,r[2],next))return nullptr;
  frame=next;cached=index;return &frame;
 }
};
class FirstBank{
 std::string path;unsigned maximum=0,bankBytes=0;uint32_t selected=37;
public:
 struct Row{int16_t x,y;uint16_t w,h;uint32_t offset,packed,bytes;};
 static_assert(sizeof(Row)==20);
 bool open(const std::string& path){
  this->path.clear();maximum=bankBytes=0;selected=37;auto f=openAssetFile(path.c_str());uint32_t h[4]{};if(!f||std::fseek(f.get(),0,SEEK_END))return false;const long length=std::ftell(f.get());
  if(length<16+37*20||std::fseek(f.get(),0,SEEK_SET)||std::fread(h,16,1,f.get())!=1||h[0]!=0x31504655||h[1]!=1||h[2]!=37||!h[3]||h[3]>256*256*4)return false;
  unsigned end=16+37*20;for(unsigned i=0;i<37;++i){Row r;if(std::fread(&r,20,1,f.get())!=1||r.offset!=end||r.w>256||r.h>256||r.x< -128||r.y< -128||r.x+int(r.w)>128||r.y+int(r.h)>128||r.bytes!=unsigned(r.w)*r.h*4||r.bytes>h[3]||bool(r.w)!=bool(r.h)||(!r.bytes&&r.packed)||r.packed>112*1024||end>unsigned(length)||r.packed>unsigned(length)-end)return false;end+=r.packed;}
  if(end!=unsigned(length))return false;
  this->path=path;maximum=h[3];bankBytes=unsigned(length);return true;
 }
 unsigned capacity()const{return maximum;}
 void reset(){selected=37;}
 bool load(unsigned id,uint8_t* decoded,uint8_t* packed,CreditsComposite057::Image& image){
  if(path.empty()||id>=37)return false;
  if(id==selected)return true;
  selected=37;image={};
  const auto file=openAssetFile(path.c_str());if(!file)return false;
  Row r;
  if(std::fseek(file.get(),0,SEEK_END)||std::ftell(file.get())!=long(bankBytes)||std::fseek(file.get(),16+20*id,SEEK_SET)||std::fread(&r,20,1,file.get())!=1||
     r.w>256||r.h>256||r.x< -128||r.y< -128||r.x+int(r.w)>128||r.y+int(r.h)>128||r.bytes!=unsigned(r.w)*r.h*4||r.bytes>maximum||bool(r.w)!=bool(r.h)||
     r.offset<16+37*20||r.offset>bankBytes||r.packed>112*1024||r.packed>bankBytes-r.offset||(!r.bytes&&r.packed)||(r.bytes&&!r.packed))return false;
  if(r.bytes){if(std::fseek(file.get(),r.offset,SEEK_SET)||std::fread(packed,r.packed,1,file.get())!=1)return false;unsigned long n=r.bytes,c=r.packed;if(uncompress2(decoded,&n,packed,&c)!=Z_OK||n!=r.bytes||c!=r.packed)return false;}
  image={decoded,r.w,r.h,r.w,float(r.x),float(r.y),false};selected=id;return true;
 }
};
template<class Gradient>void compose(uint16_t* movie,const Frame& frame,const CreditsComposite057::Image& first,const CreditsComposite057::Image& player,const CreditsComposite057::Image& ghost,bool rainbow,Gradient gradient,const CreditsComposite057::Image* ink=nullptr){
 auto visit=[&](bool foreground){const auto* p=frame.spans;for(unsigned i=0;i<frame.spanCount;++i){
  const unsigned y=u16(p),x=u16(p+2),n=u16(p+4)&0x7fff,front=u16(p+4)>>15;p+=6;
  for(unsigned j=0;j<n;++j){auto& target=movie[CreditsComposite057::swizzle(x+j,y,512,2)/2];
   if(!foreground)target=u16(p+j*2);
   else if(front){const uint32_t c=u32(p+2*n+4*j);target=CreditsComposite057::rgb565(CreditsComposite057::over(CreditsComposite057::rgba565(target),c,c>>24,true));}
  }p+=n*(front?6:2);
 }};
 auto actors=[&](bool ghosts){for(unsigned i=0;i<frame.actorCount;++i){CreditsComposite057::Actor a;std::memcpy(&a,frame.actors+32*i,32);
  if((a.kind==2)!=ghosts)continue;
  const auto image=a.kind==1?first:a.kind==2?ghost:player;
  // Source PlayerSecond has alpha1 and no modulate tracks; TheOthers has
  // constant alpha.435294 and its material assignment is commented out.
  const bool material=rainbow&&(a.kind!=0||a.alpha>.99f);
  CreditsComposite057::draw(movie,a,image,material,gradient);
  if(a.kind!=2&&ink){const float scale=a.kind==1?.6375f:.5f;a.a*=scale;a.b*=scale;a.c*=scale;a.d*=scale;CreditsComposite057::draw(movie,a,*ink,false,gradient);}
 }};
 visit(false);actors(false);visit(true);actors(true);
}
}
