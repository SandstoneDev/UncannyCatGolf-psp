#pragma once
#include "asset_file.hpp"
#include "editor_art_data.hpp"
#include "editor_menu_overlay066.hpp"
#include "editor_toolbar_layout066.hpp"
#include "editor_text_layout068.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <pspge.h>
#include <pspkernel.h>
#include <pspgu.h>
#include <zlib.h>
namespace EditorArt {
inline constexpr float Scale=17.f/32;
inline constexpr unsigned Start=0x88000,SlotBytes=262144;
struct Rect {float x,y,w,h;bool contains(float px,float py)const{return px>=x&&py>=y&&px<x+w&&py<y+h;}};
inline Rect controlRect(unsigned id){if(id>=std::size(Packed::Controls))return {};const auto&c=Packed::Controls[id];const auto r=EditorTextLayout068::rect(id,EditorToolbarLayout066::rect(id,{c.x,c.y,c.w,c.h}));return {r.x,r.y,r.w,r.h};}
inline constexpr auto staticOffsets(){std::array<unsigned,Packed::StaticPages+1> v{};v[0]=Start;for(unsigned i=0;i<Packed::StaticPages;++i)v[i+1]=v[i]+Packed::Pages[i].bytes;return v;}
inline constexpr auto Offsets=staticOffsets();
inline constexpr unsigned CacheStart=Offsets.back(),CacheSlots=(0x200000-CacheStart)/SlotBytes,End=CacheStart+CacheSlots*SlotBytes;
static_assert(CacheSlots>=2,"Editor requires at least two streamed eDRAM pages");
using Record=EditorMenuOverlay066::Record;
inline EditorMenuOverlay066::Archive menuOverlay;
inline std::array<Record,Packed::Records> records{};
inline std::array<int,CacheSlots> cached{};
inline std::array<unsigned,CacheSlots> ages{};
inline FILE* archive=nullptr;
inline bool ready=false,textureDirty=false;
inline unsigned age=0,inflateWorkspacePeak=0,inflateWorkspaceCalls=0;
inline int lastInflateError=0,lastInflateRecord=-1,preparedTab=-1;
inline uint8_t* workspace=nullptr;
using PageWriteHook=void(*)();
inline PageWriteHook pageWriteBegin=nullptr,pageWriteEnd=nullptr;
struct PageWriteScope {
 PageWriteHook end;
 PageWriteScope():end(pageWriteEnd){if(pageWriteBegin)pageWriteBegin();}
 ~PageWriteScope(){if(end)end();}
 PageWriteScope(const PageWriteScope&)=delete;
 PageWriteScope&operator=(const PageWriteScope&)=delete;
};
inline constexpr unsigned InflateWorkspaceRequired=65536;
inline uint8_t* video(unsigned offset){return static_cast<uint8_t*>(sceGeEdramGetAddr())+offset;}
struct Arena {uint8_t* memory;unsigned used=0,calls=0;};
inline voidpf allocate(voidpf opaque,uInt n,uInt size){auto& a=*static_cast<Arena*>(opaque);if(!n||!size||n>InflateWorkspaceRequired/size)return nullptr;const unsigned bytes=n*size,at=(a.used+63)&~63u;if(at>InflateWorkspaceRequired||bytes>InflateWorkspaceRequired-at)return nullptr;auto*p=a.memory+at;std::memset(p,0,bytes);a.used=at+bytes;++a.calls;return p;}
inline void release(voidpf,voidpf){}
inline unsigned inflateRecords067=0;
inline bool inflateRecord(FILE* source,const Record&r,unsigned destination){
 ++inflateRecords067;
 PageWriteScope writeScope;
 lastInflateError=-101;
 if(!source||std::fseek(source,r.offset,SEEK_SET))return false;
 z_stream z{};Arena arena{workspace};z.zalloc=allocate;z.zfree=release;z.opaque=&arena;
 int status=inflateInit(&z);if(status!=Z_OK){lastInflateError=status;return false;}
 alignas(64) uint8_t input[4096],output[4096];unsigned written=0,left=r.bytes;uLong crc=crc32(0,nullptr,0);bool good=false;
 auto* target=reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(video(destination))|0x40000000u);
 for(;;){
  if(!z.avail_in&&left){unsigned n=std::min<unsigned>(sizeof input,left);if(std::fread(input,1,n,source)!=n)break;left-=n;z.next_in=input;z.avail_in=n;}
  z.next_out=output;z.avail_out=sizeof output;status=inflate(&z,Z_NO_FLUSH);const unsigned n=sizeof output-z.avail_out;
  if((status!=Z_OK&&status!=Z_STREAM_END)||written>r.raw||n>r.raw-written)break;
  crc=crc32(crc,output,n);std::memcpy(target+written,output,n);written+=n;
  if(status==Z_STREAM_END){good=written==r.raw&&!left&&!z.avail_in&&crc==r.crc;break;}
  if(!n&&!z.avail_in&&!left)break;
 }
 inflateEnd(&z);inflateWorkspacePeak=std::max(inflateWorkspacePeak,arena.used);inflateWorkspaceCalls+=arena.calls;lastInflateError=good?0:status;return good;
}
inline bool inflatePage(unsigned index,unsigned destination){
 lastInflateRecord=int(index);
 if(!archive||index>=records.size())return false;
 if(const auto*r=menuOverlay.find(index)){
  if(inflateRecord(menuOverlay.file(),*r,destination))return true;
  // Do not publish an incomplete/corrupt optional replacement. The original
  // page overwrites the same fenced destination before any GE reader resumes.
  menuOverlay.close();
 }
 return inflateRecord(archive,records[index],destination);
}
inline void unload(){menuOverlay.close();if(archive)std::fclose(archive);archive=nullptr;ready=false;textureDirty=false;preparedTab=-1;cached.fill(-1);ages.fill(0);}
inline bool load(const char* executable,void* scratch,std::size_t scratchBytes){
 unload();workspace=nullptr;inflateWorkspacePeak=inflateWorkspaceCalls=0;
 const uintptr_t raw=reinterpret_cast<uintptr_t>(scratch),aligned=(raw+63u)&~uintptr_t(63u);
 if(!scratch||raw>UINTPTR_MAX-63u||scratchBytes<aligned-raw||scratchBytes-(aligned-raw)<InflateWorkspaceRequired){lastInflateError=-104;return false;}
 workspace=reinterpret_cast<uint8_t*>(aligned);
 if(unsigned(sceGeEdramGetSize())<End)return false;
 const auto path=companion(executable,"data/world/editor-art.bin");auto input=openAssetFile(path.c_str());uint32_t h[4]{};
 if(!input||std::fread(h,sizeof h,1,input.get())!=1||h[0]!=0x44454355||h[1]!=1||h[2]!=Packed::StaticPages||h[3]!=Packed::Records||std::fread(records.data(),sizeof records,1,input.get())!=1||std::fseek(input.get(),0,SEEK_END))return false;
 const long length=std::ftell(input.get());if(length<0)return false;unsigned end=sizeof h+sizeof records;
 for(unsigned i=0;i<records.size();++i){const auto&r=records[i];if(!r.bytes||r.offset!=end||r.raw!=Packed::Pages[i].bytes||r.format!=Packed::Pages[i].format||r.offset>unsigned(length)||r.bytes>unsigned(length)-r.offset)return false;end+=r.bytes;}
 if(end!=unsigned(length))return false;
 archive=input.release();sceGuSync(GU_SYNC_FINISH,GU_SYNC_WHAT_DONE);
 const auto overlayPath=companion(executable,"data/world/editor-menu-readable066.bin");
 auto overlayInput=openAssetFile(overlayPath.c_str());
 if(overlayInput)menuOverlay.openOwned(overlayInput.release());
 for(unsigned i=0;i<Packed::StaticPages;++i)if(!inflatePage(i,Offsets[i])){unload();return false;}
 ready=true;textureDirty=true;return true;
}
inline int resident(unsigned page){if(page<Packed::StaticPages)return int(Offsets[page]);for(unsigned i=0;i<CacheSlots;++i)if(cached[i]==int(page))return int(CacheStart+i*SlotBytes);return -1;}
inline bool preparePages(const unsigned* pages,unsigned count){
 if(!ready)return false;
 unsigned unique[CacheSlots]{},n=0;
 for(unsigned j=0;j<count;++j){const unsigned p=pages[j];if(p>=Packed::Records)return false;if(p<Packed::StaticPages)continue;bool found=false;for(unsigned i=0;i<n;++i)found|=unique[i]==p;if(!found){if(n==CacheSlots){lastInflateError=-105;return false;}unique[n++]=p;}}
 bool allResident=true;
 for(unsigned j=0;j<n;++j){bool found=false;for(unsigned i=0;i<CacheSlots;++i)found|=cached[i]==int(unique[j]);allResident&=found;}
 // No VRAM write is possible on the hot path. Keep every cold-page fence.
 if(!allResident)sceGuSync(GU_SYNC_FINISH,GU_SYNC_WHAT_DONE);
 for(unsigned j=0;j<n;++j){int slot=-1;for(unsigned i=0;i<CacheSlots;++i)if(cached[i]==int(unique[j]))slot=int(i);
  if(slot<0){unsigned oldest=~0u;for(unsigned i=0;i<CacheSlots;++i){bool needed=false;for(unsigned k=0;k<n;++k)needed|=cached[i]==int(unique[k]);if(!needed&&(cached[i]<0||ages[i]<oldest)){slot=int(i);oldest=ages[i];if(cached[i]<0)break;}}if(slot<0)return false;cached[slot]=-1;if(!inflatePage(unique[j],CacheStart+unsigned(slot)*SlotBytes))return false;cached[slot]=int(unique[j]);textureDirty=true;}
  ages[slot]=++age;
 }
 return true;
}
// Read-only check: callers can retain an open list only if no page may change.
inline bool frameResident(unsigned id){
 if(!ready||id>=std::size(Packed::Frames))return false;
 const auto&f=Packed::Frames[id];
 for(unsigned j=0;j<f.count;++j)if(resident(Packed::Parts[f.first+j].page)<0)return false;
 return true;
}
inline bool prepareFrame(unsigned id){if(id>=std::size(Packed::Frames))return false;const auto&f=Packed::Frames[id];unsigned pages[CacheSlots]{};unsigned n=0;for(unsigned j=0;j<f.count;++j){unsigned p=Packed::Parts[f.first+j].page;if(p<Packed::StaticPages)continue;bool seen=false;for(unsigned i=0;i<n;++i)seen|=pages[i]==p;if(!seen){if(n==CacheSlots)return false;pages[n++]=p;}}return preparePages(pages,n);}
inline unsigned uiBackground(unsigned tab,bool ru){return tab==0?Packed::Backgrounds[5]:tab==4?Packed::Backgrounds[2]:(ru?Packed::BackgroundsRu[tab]:Packed::Backgrounds[tab]);}
inline bool prepareUi(unsigned tab,bool ru=false){if(tab>=6)return false;if(!prepareFrame(uiBackground(tab,ru)))return false;preparedTab=int(tab);return true;}
inline bool prepareColorPicker(){return prepareFrame(Packed::ColorPicker);}
inline bool preparePopupPanel(){return prepareFrame(Packed::PopupPanel);}
inline bool prepareEditorMenu(bool ru,unsigned selected,unsigned state){return prepareFrame(Packed::EditorMenus[unsigned(ru)][selected%2][state%3]);}
inline void palette(){auto*raw=static_cast<uint8_t*>(sceGuGetMemory(1087));auto*p=reinterpret_cast<uint32_t*>((reinterpret_cast<uintptr_t>(raw)+63)&~uintptr_t(63));for(unsigned i=0;i<256;++i)p[i]=(i<<24)|0xffffffu;sceKernelDcacheWritebackRange(p,1024);sceGuClutMode(GU_PSM_8888,0,255,0);sceGuClutLoad(32,p);}
inline std::array<int,4> uiScissor{ViewX,0,ViewX+408,272};
inline void scissor(int x,int y,int right,int bottom){uiScissor={x,y,right,bottom};sceGuScissor(x,y,right,bottom);}
inline void fullView(){scissor(ViewX,0,ViewX+408,272);}
inline void frame(unsigned id,float x,float y,float sx=1,float sy=1,float angle=0,uint32_t tint=White,bool bindAlphaPalette=true){
 if(!ready||id>=std::size(Packed::Frames))return;
 if(textureDirty){sceGuTexFlush();textureDirty=false;}
 const auto&f=Packed::Frames[id];float sn=std::sin(angle),cs=std::cos(angle);
 for(unsigned j=0;j<f.count;++j){const auto&p=Packed::Parts[f.first+j];int at=resident(p.page);if(at<0)continue;unsigned fmt=Packed::Pages[p.page].format;if(fmt==5&&bindAlphaPalette)palette();setTexture(fmt,256,256,video(unsigned(at)));sceGuTexFilter(GU_NEAREST,GU_NEAREST);auto*v=static_cast<TextureVertex*>(sceGuGetMemory(4*sizeof(TextureVertex)));
  for(unsigned c=0;c<4;++c){float dx=(p.x+(c&1?p.w:0))*sx,dy=(p.y+(c&2?p.h:0))*sy;v[c]={float(p.u+(c&1?p.width:0)),float(p.v+(c&2?p.height:0)),tint,ViewX+(x+dx*cs-dy*sn)*Scale,(y+dx*sn+dy*cs)*Scale,0};}sceGuDrawArray(GU_TRIANGLE_STRIP,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,4,nullptr,v);
 }
}
inline unsigned codepoint(const char*&s){unsigned c=static_cast<unsigned char>(*s++);if(c<128)return c;unsigned n=c<224?1:c<240?2:3;c&=n==1?31:n==2?15:7;while(n--&&*s)c=(c<<6)|(static_cast<unsigned char>(*s++)&63);return c;}
inline const Packed::Glyph* glyph(unsigned font,unsigned cp){for(const auto&g:Packed::Glyphs)if(g.font==font&&g.code==cp)return &g;return nullptr;}
using TextHook=void(*)(unsigned,const char*,float,float,uint32_t,float);
inline TextHook textHook=nullptr;
using ScaledTextHook=void(*)(unsigned,const char*,float,float,uint32_t,float,float);
inline ScaledTextHook scaledTextHook=nullptr;
inline void textScaled(unsigned font,const char*s,float x,float baseline,uint32_t tint,float maxWidth,float scale){if(scaledTextHook)scaledTextHook(font,s,x,baseline,tint,maxWidth,scale);else if(textHook)textHook(font,s,x,baseline,tint,maxWidth);}
inline bool(*controlTextHook)(unsigned,float,float,const char*,bool,bool,bool)=nullptr;
inline float(*textWidthHook)(unsigned,const char*)=nullptr;
inline float textWidth(unsigned font,const char*s){if(textWidthHook)return textWidthHook(font,s);float w=0;while(s&&*s){const auto*g=glyph(font,codepoint(s));if(g)w+=g->advance;}return w;}
inline void text(unsigned font,const char*s,float x,float baseline,uint32_t tint=White,float maxWidth=1e30f){if(textHook){textHook(font,s,x,baseline,tint,maxWidth);return;}float start=x;bool bound=false;while(s&&*s){if(x-start>=maxWidth)break;unsigned cp=codepoint(s);if(cp==10){x=start;baseline+=font<std::size(Packed::FontSizes)?Packed::FontSizes[font]*1.2f:16;continue;}const auto*g=glyph(font,cp);if(g){if(!bound){palette();bound=true;}frame(g->frame,x,baseline,1,1,0,tint,false);x+=g->advance;}}}
inline bool(*readableLabelHook)(unsigned,const Packed::Label&)=nullptr;
inline void drawUi(unsigned tab,bool ru){if(tab>=6)return;fullView();frame(uiBackground(tab,ru),0,0);for(unsigned i=0;i<std::size(Packed::Labels);++i){const auto&l=Packed::Labels[i];if(!l.welcome&&!l.baked&&(l.tab<0||l.tab==int(tab))&&(!readableLabelHook||!readableLabelHook(i,l)))frame(l.frames[unsigned(ru)],l.x,l.y);}}
inline void drawControlAt(unsigned id,float x,float y,const char* value=nullptr,bool checked=false,bool hover=false,bool pressed=false,bool ru=false,bool placeholder=false){
 (void)ru;if(id>=std::size(Packed::Controls))return;
 const auto savedClip068=uiScissor;
 struct RestoreClip068{std::array<int,4>v;~RestoreClip068(){scissor(v[0],v[1],v[2],v[3]);}}restore068{savedClip068};
 if(EditorTextLayout068::sidebar(id)&&EditorTextLayout068::scrolling(EditorTextLayout068::controlTab(id)))scissor(savedClip068[0],std::max(savedClip068[1],int(EditorTextLayout068::Top*Scale)),savedClip068[2],std::min(savedClip068[3],int(EditorTextLayout068::Bottom*Scale)));
const auto&c=Packed::Controls[id];unsigned state=pressed?2:hover?1:0;if(!std::strcmp(c.type,"CheckBox")||!std::strcmp(c.type,"ColorPickerButton"))state=checked?1:0;auto r=EditorTextLayout068::rect(id,EditorToolbarLayout066::rect(id,{x,y,c.w,c.h}));if(x!=c.x||y!=c.y){r.x=x;r.y=y;}x=r.x;y=r.y;if(!std::strcmp(c.type,"SpinBox")){
  // Source captures retain the clamped minimum (2 / 128) in SpinBox text.
  // Reuse the original blank LineEdit skin and preserve the original arrows.
  const auto clip=uiScissor;const int split=ViewX+int(std::ceil((x+r.w-18)*Scale-.5f));
  if(clip[0]<std::min(clip[2],split)){scissor(clip[0],clip[1],std::min(clip[2],split),clip[3]);const auto&blank=Packed::Controls[1];frame(blank.skins[0],x,y,(r.w-18)/blank.w,r.h/blank.h);}
  if(std::max(clip[0],split)<clip[2]){scissor(std::max(clip[0],split),clip[1],clip[2],clip[3]);frame(c.skins[state],x+r.w-c.w,y,1,r.h/c.h);}
  scissor(clip[0],clip[1],clip[2],clip[3]);
 }else frame(c.skins[state],x,y,r.w/c.w,r.h/c.h);const char*s=value?value:c.text;if(!s||!*s||!std::strcmp(c.type,"ItemList")||!std::strcmp(c.type,"ColorPickerButton"))return;
 if(controlTextHook&&controlTextHook(id,x,y,s,hover,pressed||(checked&&!std::strcmp(c.type,"CheckBox")),placeholder))return;
 const bool center=!std::strcmp(c.type,"Button");float tx=x+(center?(r.w-textWidth(c.font,s))*.5f:4),baseline=y+(r.h+Packed::FontSizes[c.font])*.5f-3;
 const auto previous=uiScissor;const int left=std::max(previous[0],ViewX+int(std::ceil((x+2)*Scale))),top=std::max(previous[1],int(std::ceil(y*Scale))),right=std::min(previous[2],ViewX+int(std::floor((x+r.w-2)*Scale))),bottom=std::min(previous[3],int(std::floor((y+r.h)*Scale)));
 if(left<right&&top<bottom){scissor(left,top,right,bottom);text(c.font,s,tx,baseline,White,std::max(0.f,r.w-6));scissor(previous[0],previous[1],previous[2],previous[3]);}
}
inline void drawControl(unsigned id,const char* value=nullptr,bool checked=false,bool hover=false,bool pressed=false,bool ru=false,bool placeholder=false){if(id<std::size(Packed::Controls)){const auto&c=Packed::Controls[id];drawControlAt(id,c.x,c.y,value,checked,hover,pressed,ru,placeholder);}}
inline void drawColorPicker(float x,float y){frame(Packed::ColorPicker,x,y);}
inline void drawPopupPanel(float x,float y){frame(Packed::PopupPanel,x,y);}
inline void drawEditorMenu(bool ru,unsigned selected,unsigned state){frame(Packed::EditorMenus[unsigned(ru)][selected%2][state%3],0,0);}
inline void drawWelcome(bool ru){for(const auto&l:Packed::Labels)if(l.welcome)frame(l.frames[unsigned(ru)],l.x,l.y);for(unsigned i=0;i<std::size(Packed::Controls);++i)if(!std::strncmp(Packed::Controls[i].path,"pickOption/",11))drawControl(i,nullptr,false,false,false,ru);}
inline bool prepareImageRegion(unsigned image,Rect region){if(image>=std::size(Packed::Images))return false;const auto&f=Packed::Frames[Packed::Images[image].frame];unsigned pages[CacheSlots]{},n=0;for(unsigned j=0;j<f.count;++j){const auto&p=Packed::Parts[f.first+j];if(p.x+p.w<=region.x||p.y+p.h<=region.y||p.x>=region.x+region.w||p.y>=region.y+region.h||p.page<Packed::StaticPages)continue;bool found=false;for(unsigned i=0;i<n;++i)found|=pages[i]==p.page;if(!found){if(n==CacheSlots){lastInflateError=-105;return false;}pages[n++]=p.page;}}return preparePages(pages,n);}
inline bool prepareImage(unsigned image){if(image>=std::size(Packed::Images))return false;const auto&i=Packed::Images[image];return prepareImageRegion(image,{0,0,float(i.width),float(i.height)});}
inline unsigned imagePartCount(unsigned image){return image<std::size(Packed::Images)?Packed::Frames[Packed::Images[image].frame].count:0;}
inline Rect imagePartRegion(unsigned image,unsigned part){if(part>=imagePartCount(image))return {0,0,0,0};const auto&f=Packed::Frames[Packed::Images[image].frame];const auto&p=Packed::Parts[f.first+part];return {p.x,p.y,p.w,p.h};}
inline void drawImageRegion(unsigned image,Rect r,float x,float y,float w,float h,float angle=0,uint32_t tint=White){
 if(!ready||image>=std::size(Packed::Images)||r.w<=0||r.h<=0)return;
 if(textureDirty){sceGuTexFlush();textureDirty=false;}const auto&f=Packed::Frames[Packed::Images[image].frame];float sn=std::sin(angle),cs=std::cos(angle);
 for(unsigned j=0;j<f.count;++j){const auto&p=Packed::Parts[f.first+j];float l=std::max(p.x,r.x),t=std::max(p.y,r.y),right=std::min(p.x+p.w,r.x+r.w),bottom=std::min(p.y+p.h,r.y+r.h);if(l>=right||t>=bottom)continue;int at=resident(p.page);if(at<0)continue;setTexture(Packed::Pages[p.page].format,256,256,video(unsigned(at)));sceGuTexFilter(GU_NEAREST,GU_NEAREST);auto*v=static_cast<TextureVertex*>(sceGuGetMemory(4*sizeof(TextureVertex)));
  for(unsigned c=0;c<4;++c){float px=c&1?right:l,py=c&2?bottom:t,dx=(px-r.x)/r.w*w,dy=(py-r.y)/r.h*h;v[c]={p.u+(px-p.x)/p.w*p.width,p.v+(py-p.y)/p.h*p.height,tint,ViewX+(x+dx*cs-dy*sn)*Scale,(y+dx*sn+dy*cs)*Scale,0};}sceGuDrawArray(GU_TRIANGLE_STRIP,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,4,nullptr,v);
 }
}
inline void drawImage(unsigned image,float x,float y,float w,float h,float angle=0,uint32_t tint=White){if(image>=std::size(Packed::Images))return;const auto&i=Packed::Images[image];drawImageRegion(image,{0,0,float(i.width),float(i.height)},x,y,w,h,angle,tint);}
}
