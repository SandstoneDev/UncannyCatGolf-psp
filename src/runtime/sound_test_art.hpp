#pragma once
#include "asset_file.hpp"
#include "sound_test_art_data.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <pspge.h>
#include <pspkernel.h>
#include <pspgu.h>
#include <zlib.h>

namespace SoundTestArt {
inline constexpr float Scale=17.f/32;
inline constexpr unsigned Start=0x88000,Count=std::size(Packed::Pages);
struct Rect {float x,y,w,h;constexpr bool contains(float px,float py)const{return px>=x&&py>=y&&px<x+w&&py<y+h;}};
inline constexpr Rect ListRect{496,16,256,352},ScrollRect{748,16,4,352},BackRect{264,8,64,32},PauseRect{688,384,66,66},MixRect{248,97,128,128};
inline constexpr unsigned RowStep=100,RowHeight=96,ScrollMax=5444,ContentHeight=5796;
inline Rect rowButton(unsigned row,float scroll){return {680,32+row*100-scroll,64,64};}
struct Record {uint32_t offset,bytes,raw,format,crc;};
inline std::array<Record,Packed::Records> records{};
inline FILE* archive=nullptr;
inline bool ready=false,textureDirty=false;
inline int selectedSong=-1,selectedCostume=-1;
inline bool blankCostume=false;
inline constexpr auto offsets(){std::array<unsigned,Count+1> r{};r[0]=Start;for(unsigned i=0;i<Count;++i)r[i+1]=r[i]+Packed::Pages[i].bytes;return r;}
inline constexpr auto Offsets=offsets();
inline constexpr unsigned SelectionSlot=Offsets.back(),PlayerSlot=SelectionSlot+65536,GhostSlot=PlayerSlot+262144,End=GhostSlot+262144;
static_assert(End<=0x200000);
inline uint8_t* video(unsigned offset){return static_cast<uint8_t*>(sceGeEdramGetAddr())+offset;}
inline void unload(){if(archive)std::fclose(archive);archive=nullptr;ready=false;textureDirty=false;selectedSong=-1;selectedCostume=-1;}
inline int lastInflateError=0;
inline unsigned lastInflateRecord=0;
// Caller-owned scratch: one64KiB arena, with no zlib heap allocations.
// Only the pointer and peak/call counters persist; no backing array is added here.
inline constexpr std::size_t InflateWorkspaceRequired=64*1024;
inline uint8_t* inflateWorkspace=nullptr;
inline unsigned inflateWorkspacePeak=0,inflateWorkspaceCalls=0;
struct InflateArena {uint8_t* memory;std::size_t used=0;unsigned calls=0;};
inline voidpf workspaceAllocate(voidpf opaque,uInt items,uInt size){
 auto& arena=*static_cast<InflateArena*>(opaque);
 if(!items||!size||items>InflateWorkspaceRequired/size)return nullptr;
 const std::size_t bytes=std::size_t(items)*size,at=(arena.used+63u)&~std::size_t(63u);
 if(at>InflateWorkspaceRequired||bytes>InflateWorkspaceRequired-at)return nullptr;
 auto* result=arena.memory+at;std::memset(result,0,bytes);
 arena.used=at+bytes;++arena.calls;
 return result;
}
inline void workspaceRelease(voidpf,voidpf){}
inline bool configureInflateWorkspace(void* memory,std::size_t bytes){
 inflateWorkspace=nullptr;inflateWorkspacePeak=inflateWorkspaceCalls=0;
 if(!memory&&!bytes)return true; // Explicit legacy/default allocator path.
 if(!memory)return false;
 const auto address=reinterpret_cast<uintptr_t>(memory);
 if(address>UINTPTR_MAX-63u)return false;
 const auto aligned=(address+63u)&~uintptr_t(63u);
 const auto padding=std::size_t(aligned-address);
 if(bytes<padding||bytes-padding<InflateWorkspaceRequired)return false;
 inflateWorkspace=reinterpret_cast<uint8_t*>(aligned);return true;
}
inline bool inflatePage(unsigned index,unsigned destination){
 lastInflateError=0;lastInflateRecord=index;
 if(!archive||index>=records.size()){lastInflateError=-101;return false;}
 const auto& record=records[index];
 if(std::fseek(archive,record.offset,SEEK_SET)!=0){lastInflateError=-102;return false;}
 z_stream stream{};InflateArena arena{inflateWorkspace};
 if(inflateWorkspace){stream.zalloc=workspaceAllocate;stream.zfree=workspaceRelease;stream.opaque=&arena;}
 const int initialized=inflateInit(&stream);
 inflateWorkspacePeak=std::max(inflateWorkspacePeak,static_cast<unsigned>(arena.used));
 const unsigned initializedCalls=arena.calls;inflateWorkspaceCalls+=initializedCalls;
 if(initialized!=Z_OK){lastInflateError=initialized;return false;}
 alignas(64) uint8_t input[4096],output[4096];
 unsigned written=0,remaining=record.bytes;uLong crc=crc32(0,nullptr,0);
 auto* target=reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(video(destination))|0x40000000u);
 bool good=false;
 for(;;){
  if(!stream.avail_in&&remaining){const unsigned n=std::min<unsigned>(sizeof input,remaining);if(std::fread(input,1,n,archive)!=n){lastInflateError=-103;break;}remaining-=n;stream.next_in=input;stream.avail_in=n;}
  stream.next_out=output;stream.avail_out=sizeof output;
  const int status=inflate(&stream,Z_NO_FLUSH);lastInflateError=status;const unsigned n=sizeof output-stream.avail_out;
  if((status!=Z_OK&&status!=Z_STREAM_END)||written>record.raw||n>record.raw-written)break;
  crc=crc32(crc,output,n);std::memcpy(target+written,output,n);written+=n;
  if(status==Z_STREAM_END){good=written==record.raw&&!remaining&&!stream.avail_in&&crc==record.crc;break;}
  if(!n&&!stream.avail_in&&!remaining)break;
 }
 inflateEnd(&stream);
 inflateWorkspacePeak=std::max(inflateWorkspacePeak,static_cast<unsigned>(arena.used));
 inflateWorkspaceCalls+=arena.calls-initializedCalls;
 if(good)lastInflateError=0;
 return good;
}
inline bool select(int song){
 if(!ready||song<-1||song>=58)return false;
 if(song==selectedSong)return true;
 sceGuSync(GU_SYNC_FINISH,GU_SYNC_WHAT_DONE);
 selectedSong=-1;
 if(song>=0&&!inflatePage(Packed::SelectionRecord+song,SelectionSlot))return false;
 selectedSong=song;textureDirty=true;return true;
}
inline bool selectCostume(unsigned costume,unsigned copycatBase=0){
 if(!ready||costume>=24||copycatBase>24)return false;
 if(costume==2)costume=copycatBase==2?0:copycatBase;
 const bool blank=costume==24; if(blank)costume=0;
 blankCostume=blank;
 if(selectedCostume==static_cast<int>(costume))return true;
 sceGuSync(GU_SYNC_FINISH,GU_SYNC_WHAT_DONE);
 selectedCostume=-1;
 if(!inflatePage(Packed::Costumes[costume][0].record,PlayerSlot)||!inflatePage(Packed::Costumes[costume][1].record,GhostSlot))return false;
 selectedCostume=costume;textureDirty=true;return true;
}
inline bool load(const char* executable,void* workspace=nullptr,std::size_t workspaceBytes=0){
 unload();
 if(!configureInflateWorkspace(workspace,workspaceBytes)){lastInflateError=-104;lastInflateRecord=0;return false;}
 if(static_cast<unsigned>(sceGeEdramGetSize())<End)return false;
 const auto path=companion(executable,"data/world/sound-test-art.bin");
 auto input=openAssetFile(path.c_str());uint32_t header[4]{};
 if(!input||std::fread(header,sizeof header,1,input.get())!=1||header[0]!=0x54534355||header[1]!=1||header[2]!=Count||header[3]!=records.size()||std::fread(records.data(),sizeof records,1,input.get())!=1||std::fseek(input.get(),0,SEEK_END)!=0)return false;
 const long length=std::ftell(input.get());if(length<0)return false;
 unsigned end=sizeof header+sizeof records;
 for(unsigned i=0;i<records.size();++i){const auto&r=records[i];const unsigned raw=i<Count?Packed::Pages[i].bytes:i<Packed::CostumeRecord?65536:262144,fmt=i<Count?Packed::Pages[i].format:i<Packed::CostumeRecord?5:3;
  if(!r.bytes||r.offset!=end||r.raw!=raw||r.format!=fmt||r.offset>static_cast<unsigned long>(length)||r.bytes>static_cast<unsigned long>(length)-r.offset)return false;
  end+=r.bytes;
 }
 if(end!=static_cast<unsigned long>(length))return false;
 archive=input.release();sceGuSync(GU_SYNC_FINISH,GU_SYNC_WHAT_DONE);
 for(unsigned i=0;i<Count;++i)if(!inflatePage(i,Offsets[i])){unload();return false;}
 ready=true;textureDirty=true;
 if(!selectCostume(0)){unload();return false;}return true;
}
inline void fullView(){sceGuScissor(ViewX,0,ViewX+408,272);}
inline void fieldView(){sceGuScissor(ViewX+136,0,ViewX+408,272);}
inline void listView(){sceGuScissor(ViewX+int(496*Scale),int(16*Scale),ViewX+int(752*Scale),int(368*Scale));}
inline void flush(){if(textureDirty){sceGuTexFlush();textureDirty=false;}}
inline void whitePalette(){
 auto* raw=static_cast<uint8_t*>(sceGuGetMemory(256*4+63));auto* palette=reinterpret_cast<uint32_t*>((reinterpret_cast<uintptr_t>(raw)+63)&~uintptr_t(63));
 for(unsigned i=0;i<256;++i)palette[i]=(i<<24)|0xffffffu;
 sceKernelDcacheWritebackRange(palette,1024);sceGuClutMode(GU_PSM_8888,0,255,0);sceGuClutLoad(32,palette);
}
inline void frame(unsigned index,float x,float y,float sx=1,float sy=1,float angle=0,uint32_t tint=White){
 if(!ready||index>=std::size(Packed::Frames))return;
 flush();
 const auto& image=Packed::Frames[index];const float sn=std::sin(angle),cs=std::cos(angle);
 sceGuTexFilter(GU_NEAREST,GU_NEAREST);
 unsigned previous=~0u;
 for(unsigned i=image.start;i<image.start+image.count;++i){
  const auto&p=Packed::Parts[i];const unsigned fmt=p.page==Count?5:Packed::Pages[p.page].format;
  if(fmt==5&&previous!=5)whitePalette();
  previous=fmt;
  setTexture(fmt,256,256,video(p.page==Count?SelectionSlot:Offsets[p.page]));
  auto*v=static_cast<TextureVertex*>(sceGuGetMemory(4*sizeof(TextureVertex)));
  for(unsigned c=0;c<4;++c){const float dx=(p.x+(c&1?p.w:0))*sx,dy=(p.y+(c&2?p.h:0))*sy;v[c]={float(p.u+(c&1?p.width:0)),float(p.v+(c&2?p.height:0)),tint,ViewX+(x+dx*cs-dy*sn)*Scale,(y+dx*sn+dy*cs)*Scale,0};}
  sceGuDrawArray(GU_TRIANGLE_STRIP,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,4,nullptr,v);
 }
}
inline void box(float x,float y,float w,float h,uint32_t color){rectangle(ViewX+x*Scale,y*Scale,w*Scale,h*Scale,color);}
inline unsigned speakerFrame(float seconds,float speed,bool playing){return playing?std::min(3u,static_cast<unsigned>(std::max(0.f,seconds*10*speed))%10):0;}
inline unsigned cdFrame(float seconds){return static_cast<unsigned>(std::max(0.f,seconds*15))%8;}
inline unsigned pokerFrame(float age,bool pauseMix){unsigned i=std::min(2u,static_cast<unsigned>(std::max(0.f,age*20)));return pauseMix?i:2-i;}
inline void drawBackground(float seconds,unsigned speaker,float twinkleGain=0,const float* bands=nullptr){
 fieldView();box(256,0,512,512,0xff000000u);
 for(unsigned i=0;i<16;++i)frame(Packed::Speaker0+std::min(3u,speaker),256+128*(i%4),128*(i/4),1,1,0,0xff808080u);
 constexpr float colors[7][3]={{.143348f,.0134638f,.00688038f},{.141027f,.0599234f,.00309231f},{.125491f,.0932884f,.00108901f},{.0386277f,.0852165f,.00544652f},{0,.0625715f,.0847927f},{.0597771f,.0234842f,.0946816f},{.114036f,0,.0511524f}};
 const float t=std::fmod(std::max(0.f,seconds),14.f)*.5f;const unsigned a=static_cast<unsigned>(t),b=(a+1)%7;const float f=t-a;
 const auto ch=[&](unsigned channel){
  const float p=colors[(a+6)%7][channel],q=colors[a][channel],r=colors[b][channel],s=colors[(a+2)%7][channel];
  const float value=.5f*(2*q+(-p+r)*f+(2*p-5*q+4*r-s)*f*f+(-p+3*q-3*r+s)*f*f*f);
  return unsigned(std::clamp(value,0.f,1.f)*255+.5f);
 };
 box(256,0,512,512,GU_RGBA(ch(0),ch(1),ch(2),85));
 for(unsigned i=0;i<12&&twinkleGain>0;++i){
  const float time=std::max(0.f,seconds)+i/12.f,life=time-std::floor(time);
  unsigned seed=unsigned(std::floor(time))*747796405u+i*2891336453u+277803737u;
  const auto random=[&](){seed=seed*1664525u+1013904223u;return float(seed>>8)*(1.f/16777216.f);};
  const float x=(random()-.5f)*512,y=(random()-.5f)*512,angle=random()*6.2831853f,spin=(random()*2-1)*6.2831853f;
  const float orbit=(random()*.5f-.25f)*6.2831853f*life,radius=(random()*512-256)*life*life*.5f,tangent=(random()*512-256)*life*life*.5f;
  const float inv=1/std::max(1.f,std::sqrt(x*x+y*y)),dx=x*inv,dy=y*inv,cs=std::cos(orbit),sn=std::sin(orbit);
  const float u=life<.5f?life*2:(life-.5f)*2,h00=2*u*u*u-3*u*u+1,h10=u*u*u-2*u*u+u,h01=-2*u*u*u+3*u*u,h11=u*u*u-u*u;
  const float curve=life<.5f?h10*.01393205f+h01:h00-h11*2.989455f;
  const float size=std::max(0.f,curve)*twinkleGain*(1+random());
  frame(Packed::Twinkle,512+x*cs-y*sn+dx*radius-dy*tangent,256+x*sn+y*cs+dy*radius+dx*tangent,size,size,angle+spin*life);
 }
 constexpr uint32_t tints[]={0xff586cff,0xff548fff,0xff46cbff,0xff6effaa,0xffd9ff63,0xffffc085,0xffff9fcf,0xffc87dff};
 for(unsigned i=0;i<8;++i)frame(Packed::MiniCanny,464+40*i,487.872f-(bands?bands[i]*512:0),1,1,0,tints[i]);
}
inline void drawList(float scroll,int selected,int hover,int pressed,bool ru,const bool* unlocked,bool drawScrollbar=true){
 (void)selected;(void)hover;(void)ru;scroll=std::clamp(scroll,0.f,float(ScrollMax));listView();
 for(unsigned i=0;i<58;++i){const float y=16+i*100-scroll;if(y+96<=16||y>=368)continue;const bool open=!unlocked||unlocked[i];frame(open?Packed::List[i]:Packed::Locked[i],496,y);if(open)frame(pressed==int(i)?Packed::ListPressed:Packed::ListNormal,680,y+16,1,1,0,Packed::Colors[i]);}
 if(drawScrollbar){fullView();frame(Packed::ScrollTrack,744,12);const float y=16+scroll/ScrollMax*(352-352.f*352/5796);frame(Packed::Grab0,744,y-4);}
}
inline void drawScroll(float scroll,unsigned state){fullView();frame(Packed::ScrollTrack,744,12);frame(Packed::Grab0+std::min(2u,state),744,12+std::clamp(scroll,0.f,float(ScrollMax))/ScrollMax*(352-352.f*352/5796));}
inline void drawSelection(int song,bool ru){fullView();frame((ru?Packed::Now1:Packed::Now0),280,69);if(song<0){frame(Packed::EmptyName,280,93);frame(Packed::EmptyDesc,280,384);}else if(song<58&&song==selectedSong){frame(Packed::Selected[song][unsigned(ru)*2],280,93);frame(Packed::Selected[song][unsigned(ru)*2+1],280,384);}}
inline void drawControls(bool songVisible,bool paused,bool pauseMix,unsigned cd,unsigned poker,float animationAge,bool ru,float seconds=0){
 fullView();frame(Packed::CD0+cd%8,376,232,1,1,0,selectedSong>=0?Packed::Colors[selectedSong]:White);
 frame(Packed::Poker0+std::min(2u,poker),346,195,1,-1,-1.57079f);
 frame(Packed::Back0+static_cast<unsigned>(std::max(0.f,seconds*30))%Packed::BackFrames,296,24);
 frame((ru?Packed::BackLabel1:Packed::BackLabel0),272,8);
 if(pauseMix)frame((ru?Packed::Mix1:Packed::Mix0),264,176,1,1,-.0698132f);
 if(songVisible){const float age=std::max(0.f,animationAge),pulse=age<.0666667f?1+age*3:age<.2f?1.2f-(age-.0666667f)*1.5f:1;const bool playIcon=age<.1f?!paused:paused;frame(playIcon?Packed::Play:Packed::Pause,721-33*pulse,417-33*pulse,pulse,pulse);}
}
struct DancerPose {float x,y,sx,sy,skew;};
inline DancerPose dancerPose(float seconds,bool paused){
 if(paused)return {376,322,.16f,.16f,0};
 float t=std::fmod(std::max(0.f,seconds),.8f)*5;unsigned i=static_cast<unsigned>(t);
 constexpr float x[]={392,376,368,376},y[]={322,338,322,338},skew[]={.324631f,0,-.324631f,0};
 const float k=skew[i%4];
 return {x[i%4],y[i%4],i%2?.171875f:.109375f,i%2?.0886361f:.160156f,k};
}
inline uint32_t rainbow(float x,float seconds){
 float phase=x/768-seconds*.5f;phase-=std::floor(phase);
 return Packed::Rainbow[static_cast<unsigned>(phase*64)%64];
}
inline void dancerTexture(const DancerPose& pose,const void* texture,unsigned width,unsigned height,unsigned originalW,unsigned originalH,bool colored,float seconds,unsigned format=GU_PSM_8888){
 fullView();flush();setTexture(format,width,height,texture);sceGuTexFilter(GU_NEAREST,GU_NEAREST);
 const float sn=std::sin(pose.skew),cs=std::cos(pose.skew);const unsigned strips=colored?64:1;
 for(unsigned s=0;s<strips;++s){auto*v=static_cast<TextureVertex*>(sceGuGetMemory(4*sizeof(TextureVertex)));for(unsigned c=0;c<4;++c){const float u=float(s+(c&1?1:0))/strips,dx=(u-.5f)*originalW*pose.sx,dy=((c&2?1.f:0.f)-.5f)*originalH*pose.sy,x=pose.x+dx-dy*sn,y=pose.y+dy*cs;v[c]={u*width,float(c&2?height:0),colored?rainbow(x,seconds):White,ViewX+x*Scale,y*Scale,0};}sceGuDrawArray(GU_TRIANGLE_STRIP,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,4,nullptr,v);}
}
inline void drawDancer(const DancerPose& pose,bool ghost,float seconds){if(!ready||selectedCostume<0||(!ghost&&blankCostume))return;const auto&c=Packed::Costumes[selectedCostume][unsigned(ghost)];dancerTexture(pose,video(ghost?GhostSlot:PlayerSlot),256,256,c.width,c.height,c.rainbow,seconds);}
// ArtistCopycat stores swizzled T8 indices, with the24-entry ArtistInk palette.
// Palette storage belongs to this GE list; later indexed text cannot overwrite it.
inline void drawDancerInk(const DancerPose& pose,const uint8_t* pixels,const uint32_t* colors){
 if(!ready||!pixels||!colors)return;
 auto* raw=static_cast<uint8_t*>(sceGuGetMemory(24*sizeof(uint32_t)+63));
 auto* palette=reinterpret_cast<uint32_t*>((reinterpret_cast<uintptr_t>(raw)+63)&~uintptr_t(63));
 std::memcpy(palette,colors,24*sizeof(uint32_t));
 sceKernelDcacheWritebackRange(palette,24*sizeof(uint32_t));
 sceGuClutMode(GU_PSM_8888,0,255,0);sceGuClutLoad(3,palette);
 dancerTexture(pose,pixels,128,128,512,512,false,0,GU_PSM_T8);
}
inline void cursor(float x,float y){fullView();frame(Packed::Cursor,x,y);}
}
