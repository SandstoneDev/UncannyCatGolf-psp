#pragma once
// Included after editor_render.hpp. Uses existing source-derived packed art;
// owns only bounded heap pages and never writes profile or costume preferences.
#include "menu_scroll.hpp"
#include "villa_costumes054.hpp"
#include "cat_artist_palette.hpp"
#include "par_ball.hpp"
#include "thought_bank.hpp"
#include "course_objects.hpp"
#include "editor_hud_assets.hpp"
#include <memory>
#include <new>
#include <zlib.h>
namespace EditorPlayHud {
using Vec2=ucg::editor::Vec2;
using Color=ucg::editor::Color;
struct Buffer {
  std::unique_ptr<uint8_t[]> owner;
  uint8_t* data=nullptr;
  bool allocate(unsigned n) { owner.reset(new(std::nothrow) uint8_t[n+63]); data=owner?reinterpret_cast<uint8_t*>((reinterpret_cast<uintptr_t>(owner.get())+63)&~uintptr_t(63)):nullptr; return data!=nullptr; }
  void clear(){owner.reset();data=nullptr;}
};
inline Buffer menuTile,menuHud,plate,digits,skin,ink,locale,tvPixels;
inline std::unique_ptr<ThoughtPlayer> tvPlayer;
inline bool tvReady=false,tvBegan=false;
inline uint8_t advance[LocaleGlyphs]{};
inline VisualHeader visual{};
inline VillaCostumes::Id costume=VillaCostumes::Id::Canny;
inline VillaCostumes::Pair pair{};
inline bool hasInk=false,ready=false,ru=false;
inline bool loadFontPages();
inline std::string error;
inline std::string hudRoot;
inline uint64_t shaderEpoch=0;
inline double shaderTime=0;
inline void updateTime(){const uint64_t now=sceKernelGetSystemTimeWide();if(!shaderEpoch)shaderEpoch=now;shaderTime=double(now-shaderEpoch)/1000000.;}
inline bool read(FILE* f,long at,void* dest,unsigned bytes){return f&&std::fseek(f,at,SEEK_SET)==0&&std::fread(dest,1,bytes,f)==bytes;}
inline void patch(const Patch& p,float x,float y,float sx=1,float sy=1,uint32_t tint=White){
  if(p.width<=0||p.height<=0)return;
  auto*v=static_cast<TextureVertex*>(sceGuGetMemory(2*sizeof(TextureVertex)));
  v[0]={float(p.u),float(p.v),tint,ViewX+x,y,0};
  v[1]={float(p.u+p.width),float(p.v+p.height),tint,ViewX+x+p.width*sx,y+p.height*sy,0};
  sceGuDrawArray(GU_SPRITES,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,2,nullptr,v);
}
inline bool valid(const Patch&p,unsigned w,unsigned h){return p.u>=0&&p.v>=0&&p.width>=0&&p.height>=0&&unsigned(p.u+p.width)<=w&&unsigned(p.v+p.height)<=h;}
inline void unload(){menuTile.clear();menuHud.clear();plate.clear();digits.clear();skin.clear();ink.clear();locale.clear();tvPixels.clear();tvPlayer.reset();hasInk=ready=tvReady=tvBegan=false;hudRoot.clear();}
inline bool load(const char* exe,unsigned profile,bool russian){
  unload();error.clear();ru=russian;hudRoot=companion(exe,"");shaderEpoch=sceKernelGetSystemTimeWide();shaderTime=0;
  const auto fail=[](const char* why){error=why;unload();return false;};
  if(!menuTile.allocate(MenuScroll::TileBytes)||!menuHud.allocate(MenuScroll::HudBytes)||!plate.allocate(512*272*2)||!digits.allocate(512*256*2)||!locale.allocate(512*128*4))return fail("HUD/costume page allocation failed");
  const auto font=openAssetFile(companion(exe,"data/interface/locale-font.bin").c_str());
  if(!read(font.get(),0,advance,sizeof advance)||!read(font.get(),sizeof advance,locale.data,512*128*4)||std::any_of(std::begin(advance),std::end(advance),[](uint8_t n){return !n||n>16;}))return fail("Cannot read locale font");
  const auto menu=openAssetFile(companion(exe,"data/interface/menu-scroll.pak").c_str());
  if(!menu||!MenuScroll::valid(companion(exe,"data/interface/menu-scroll.pak").c_str())||!read(menu.get(),4,menuTile.data,MenuScroll::TileBytes)||!read(menu.get(),MenuScroll::recordOffset(ru?2:0),menuHud.data,MenuScroll::HudBytes))return fail("Cannot read Master menu backdrop");
  const auto course=openAssetFile(companion(exe,ru?"data/courses/ru/0_01.bin":"data/courses/en/0_01.bin").c_str());
  constexpr long header=100, page=512*512*2;
  if(!read(course.get(),header,&visual,sizeof visual)||visual.magic!=0x32564355||!read(course.get(),header+sizeof visual+4*page,plate.data,512*272*2)||!read(course.get(),header+sizeof visual+5*page,digits.data,512*256*2))return fail("Cannot read localized LevelHUD atlas");
  for(const auto&p:visual.strokes)if(!valid(p,512,256))return fail("Invalid stroke atlas patch");
  for(const auto&row:visual.clock)for(const auto&p:row)if(!valid(p,512,256))return fail("Invalid clock atlas patch");
  using File=PreferenceFile<0x43564355,VillaCostumes::validPreference>;
  costume=static_cast<VillaCostumes::Id>(File(companion(exe,"costume.cfg")).read());
  pair=VillaCostumes::actorPair(costume);
  if(costume==VillaCostumes::Id::Copycat){
    std::unique_ptr<ArtistBook::Book> book(new(std::nothrow) ArtistBook::Book);
    if(!book)return fail("Cannot allocate Copycat book");
    ArtistBook::load(companion(exe,("cat-artist-"+std::to_string(profile)+".art").c_str()),*book);
    const auto base=book->bases[book->active];
    pair=base==ArtistBook::Blank?VillaCostumes::Pair{VillaCostumes::Art::BlankArt,VillaCostumes::row(VillaCostumes::Id::Canny).ghost}:VillaCostumes::actorPair(static_cast<VillaCostumes::Id>(base));
    const auto&canvas=book->art[book->active];hasInk=std::any_of(canvas.begin(),canvas.end(),[](uint8_t p){return p!=0;});
    if(hasInk){if(!ink.allocate(canvas.size()))return fail("Cannot allocate Copycat ink");ArtistInk::swizzle(canvas,ink.data);}
  }
  for(auto*b:{&menuTile,&menuHud,&plate,&digits})sceKernelDcacheWritebackRange(b->data,b==&menuTile?MenuScroll::TileBytes:b==&menuHud?MenuScroll::HudBytes:b==&plate?512*272*2:512*256*2);
  if(hasInk)sceKernelDcacheWritebackRange(ink.data,ArtistBook::Side*ArtistBook::Side);
  sceKernelDcacheWritebackRange(locale.data,512*128*4);
  if(!loadFontPages())return fail("Cannot read original HUD font pages");
  ready=true;return true;
}
inline unsigned tvTick(){return unsigned(shaderTime*60);}
inline void resetTV(){if(tvPlayer){tvPlayer->begin(tvTick());tvBegan=true;}}
inline void thought(unsigned category){
  if(!tvPlayer||!category||category>18)return;
  const auto value=static_cast<Thought>(category);tvPlayer->play(value,tvTick());
  if(value==Thought::Win||value==Thought::Death)tvPlayer->setFallback(value);
}
inline bool loadTV(const char*exe){
  tvPlayer.reset(new(std::nothrow) ThoughtPlayer);tvReady=tvBegan=false;
  if(!tvPlayer||!tvPlayer->open(companion(exe,"data/interface/thoughts.pak").c_str())){error="Cannot open Master television art";tvPlayer.reset();return false;}
  tvReady=true;return true;
}
inline void tv(unsigned milliseconds,bool gameplay=false){
  if(!tvReady)return;
  // Master TV continues through pause; shaderTime is the shared animation clock.
  const unsigned tick=shaderEpoch?tvTick():unsigned(uint64_t(milliseconds)*60/1000);
  if(gameplay&&!tvBegan)resetTV();
  const auto*p=gameplay?tvPlayer->frame(tick):tvPlayer->menuFrame(tick);
  if(p){setTexture(GU_PSM_5650,64,64,p);patch({0,0,58,48,0,0},21,186);}
  TvCaption059::draw();
}
// Browser world/results pages own the right viewport but retain Master's HUD.
// This draws only the original left 136 pixels and its television/caption.
inline void leftBackdrop(unsigned milliseconds){
  if(!ready)return;
  EditorArt::fullView();setTexture(GU_PSM_5650,256,512,menuHud.data);patch({0,0,136,272,0,0},0,0);
  tv(milliseconds);
}
inline void backdrop(unsigned milliseconds){
  if(!ready)return;
  EditorArt::fullView();setTexture(GU_PSM_5650,256,512,menuHud.data);patch({0,0,136,272,0,0},0,0);
  const auto scroll=MenuScroll::phase(milliseconds);setTexture(GU_PSM_5650,256,256,menuTile.data);sceGuTexFilter(GU_LINEAR,GU_LINEAR);
  for(float y=-scroll.y;y<272;y+=MenuScroll::TileSize)for(float x=136-scroll.x;x<408;x+=MenuScroll::TileSize){
    const float l=std::max(x,136.f),r=std::min(x+MenuScroll::TileSize,408.f),t=std::max(y,0.f),b=std::min(y+MenuScroll::TileSize,272.f);if(l>=r||t>=b)continue;
    auto*v=static_cast<TextureVertex*>(sceGuGetMemory(2*sizeof(TextureVertex)));
    v[0]={1+(l-x)/MenuScroll::Scale,1+(t-y)/MenuScroll::Scale,White,ViewX+l,t,0};v[1]={1+(r-x)/MenuScroll::Scale,1+(b-y)/MenuScroll::Scale,White,ViewX+r,b,0};
    sceGuDrawArray(GU_SPRITES,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,2,nullptr,v);
  }sceGuTexFilter(GU_NEAREST,GU_NEAREST);tv(milliseconds);
}
template<class Draw>inline void rainbow(float left,float right,const Draw&draw){
  const auto saved=EditorRender::clip;const int first=int(std::floor(std::max(left,saved.x))),last=int(std::ceil(std::min(right,saved.right)));if(first>=last)return;
  const auto color=[&](int x){double phase=(x+.5-ViewX)/408.-shaderTime*.5;phase-=std::floor(phase);return EditorRender::HudAssets::Rainbow[std::min(63u,unsigned(phase*64))];};
  int start=first;uint32_t tint=color(first);
  for(int x=first+1;x<=last;++x){const uint32_t next=x==last?~tint:color(x);if(next==tint)continue;EditorRender::clip.x=std::max(saved.x,float(start));EditorRender::clip.right=std::min(saved.right,float(x));draw(tint);start=x;tint=next;}
  EditorRender::clip=saved;EditorRender::applyClip();
}
inline bool costumeImage(const EditorRender::View&view,unsigned sprite,Vec2 pos,Vec2 scale,double rotation,Color tint,bool material=true){
  const auto&image=EditorRender::HudAssets::Sprites[sprite];
  if(!image.width||!image.height||!*image.path)return true;
  const EditorImages::Info info{image.width,image.height,false};const EditorArt::Rect region{0,0,float(image.width),float(image.height)};
  const double cs=std::cos(rotation),sn=std::sin(rotation);
  const auto transform=[&](Vec2 pixel){
    const bool bounded=EditorProjectionMath::withinProjectionRange(pixel.x) && EditorProjectionMath::withinProjectionRange(pixel.y) &&
      EditorProjectionMath::withinProjectionRange(pos.x) && EditorProjectionMath::withinProjectionRange(pos.y) &&
      EditorProjectionMath::withinProjectionRange(scale.x) && EditorProjectionMath::withinProjectionRange(scale.y) &&
      EditorProjectionMath::withinProjectionRange(rotation);
    if(!bounded){const double x=(pixel.x-image.width*.5)*.064*scale.x,y=(pixel.y-image.height*.5)*.064*scale.y;return view.vertex({pos.x+x*cs-y*sn,pos.y+x*sn+y*cs});}
    const double x=EditorProjectionMath::multiply(EditorProjectionMath::multiply(EditorProjectionMath::subtract(pixel.x,EditorProjectionMath::multiply(image.width,.5)),.064),scale.x),
      y=EditorProjectionMath::multiply(EditorProjectionMath::multiply(EditorProjectionMath::subtract(pixel.y,EditorProjectionMath::multiply(image.height,.5)),.064),scale.y);
    return view.vertex({EditorProjectionMath::subtract(EditorProjectionMath::add(pos.x,EditorProjectionMath::multiply(x,cs)),EditorProjectionMath::multiply(y,sn)),
      EditorProjectionMath::add(EditorProjectionMath::add(pos.y,EditorProjectionMath::multiply(x,sn)),EditorProjectionMath::multiply(y,cs))});
  };
  const auto draw=[&](uint32_t color,const EditorRender::View*lights,const Color*sourceTint=nullptr){EditorRender::customImage(hudRoot+image.path,info,region,transform,color,lights,sourceTint,EditorWholeCpu::hudSpriteAsset(sprite));};
  if(material&&VillaCostumes::Art::rainbow(sprite)){float left=1e30f,right=-1e30f;for(Vec2 corner:std::array<Vec2,4>{{{0,0},{double(image.width),0},{double(image.width),double(image.height)},{0,double(image.height)}}}){const auto p=transform(corner);left=std::min(left,p.x);right=std::max(right,p.x);}rainbow(left,right,[&](uint32_t color){draw(color,nullptr);});}
  else draw(EditorRender::rgba(tint),&view,&tint);
  return true;
}
inline bool player(const EditorRender::View&view,Vec2 pos,Vec2 scale,double rotation,Color tint){
  if(view.scene&&view.scene->dead)return true;
  if(!ready)return false;
  const auto draw=[&](const void*pixels,unsigned format,unsigned width,unsigned height,const Patch&p,double raster,const uint32_t*palette=nullptr,unsigned colors=0){
    if(p.width<=0||p.height<=0)return;
    std::array<EditorRender::Vertex,4> v{};const double cs=std::cos(rotation),sn=std::sin(rotation);
    const double xs[]={0.,double(p.width),double(p.width),0.},ys[]={0.,0.,double(p.height),double(p.height)};
    for(unsigned i=0;i<4;++i){double x=(p.x+xs[i])*raster*scale.x,y=(p.y+ys[i])*raster*scale.y;v[i]=view.vertex({pos.x+x*cs-y*sn,pos.y+x*sn+y*cs},float(p.u+xs[i]),float(p.v+ys[i]));}
    EditorRender::shadeTexture(view,static_cast<const uint8_t*>(pixels),format,width,height,v,EditorRender::rgba(tint),palette,colors,&tint);
  };
  costumeImage(view,pair.player,pos,scale,rotation,tint,costume!=VillaCostumes::Id::Copycat);
  if(hasInk)draw(ink.data,GU_PSM_T8,128,128,ArtistInk::Full,4*.064,ArtistInk::Palette,24);
  return true;
}
inline bool ghost(const EditorRender::View&view,Vec2 pos,Vec2 scale,double rotation,Color tint){return ready?costumeImage(view,pair.ghost,pos,scale,rotation,tint):false;}
inline bool ghostPlain(const EditorRender::View&view,Vec2 pos,Vec2 scale,double rotation,Color tint){return ready?costumeImage(view,pair.ghost,pos,scale,rotation,tint,false):false;}
} // namespace EditorPlayHud
#include "editor_hud_text.hpp"
namespace EditorPlayHud {
inline bool loadFontPages(){for(const auto*path:EditorRender::HudFonts::Paths){EditorImages::Info info;if(!EditorImages::inspect((hudRoot+path).c_str(),info)||info.width!=240||info.height!=240)return false;}return true;}
inline void clock(unsigned centis,unsigned first,bool hours){
  const unsigned h=centis/360000%100,m=centis/6000%60,s=centis/100%60;
  if(hours){const auto&a=visual.clock[first++][h/10];patch(a,a.x,a.y);const auto&b=visual.clock[first++][h%10];patch(b,b.x,b.y);}
  for(auto d:{m/10,m%10,s/10,s%10,centis/10%10,centis%10}){const auto&p=visual.clock[first++][d];patch(p,p.x,p.y);}
}
inline void gradient(float x,float y,float w,float h){
  setTexture(GU_PSM_5650,512,512,plate.data);auto*v=static_cast<TextureVertex*>(sceGuGetMemory(2*sizeof(TextureVertex)));
  v[0]={126,y,White,ViewX+x,y,0};v[1]={127,y+h,White,ViewX+x+w,y+h,0};
  sceGuDrawArray(GU_SPRITES,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,2,nullptr,v);
}
inline void localeText(const char*text,float x,float y,float scale=.94f,uint32_t tint=White,bool centered=false){
  if(centered){const char*p=text;unsigned width=0;while(*p)width+=advance[nextGlyph(p)];x-=width*scale*.5f;}
  setTexture(GU_PSM_8888,512,128,locale.data);
  while(*text){const auto g=nextGlyph(text);patch({int16_t(g%32*16),int16_t(g/32*20),16,20,0,0},x,y,scale,scale,tint);x+=advance[g]*scale;}
}
// Source-space name/author and numeric values; parent retains pause/result handling.
inline void draw(const EditorController::Controller&controller,const EditorPlay::Scene&scene,unsigned totalCentiseconds=0,int64_t totalScore=0,unsigned prisms=0){
  if(!ready)return;
  const auto savedClip=EditorRender::clip;EditorRender::clip={float(ViewX),0,float(ViewX+408),272};
  (void)totalCentiseconds;(void)totalScore;(void)prisms; // customPThru never instantiates PthruHUD.
  EditorArt::fullView();setTexture(GU_PSM_5650,256,512,menuHud.data);patch({0,0,136,272,0,0},0,0);
  const auto text=[&](const char*key){const auto*p=controller.model.document.root.find(key);return p?p->text:std::string();};
  const auto name=controller.model.document.root.find("levelName")?text("world")+"-"+text("id")+": "+text("levelName"):std::string("-");
  const auto author=std::string("Level by: ")+(controller.model.document.root.find("levelAuthor")?text("levelAuthor"):std::string("Name"));
  const auto&pose=EditorRender::HudFonts::IntroPoses[std::min(unsigned(std::max(0.,shaderTime-FontText::introEpoch)*60),86u)];
  FontText::label(name.c_str(),FontText::Name,pose.x,pose.y,240,80,true,false,true);
  FontText::label(author.c_str(),FontText::Author,pose.x,pose.y-16,240,24,true,true,true,-1);
  const unsigned centis=scene.elapsedTicks*5/3;
  char n[32];std::snprintf(n,sizeof n,"%02u:%02u",centis/6000,centis/100%60);
  const uint32_t timeTint=scene.elapsedTicks/60.>scene.onTime+25?0xff0000ff:scene.elapsedTicks/60.>=scene.onTime?0xff00ffff:0xff00ff00;
  const auto&timer=EditorRender::HudAssets::timer;
  const EditorImages::Info timerInfo{timer.width,timer.height,false};
  EditorRender::customImage(hudRoot+timer.path,timerInfo,{0,0,float(timer.width),float(timer.height)},[&](Vec2 p){return EditorRender::Vertex{ViewX+float(88+(p.x-timer.width*.5)*.311111)*EditorArt::Scale,float(312+(p.y-timer.height*.5)*.24)*EditorArt::Scale,0,0};},White);
  FontText::label(n,FontText::Time,40,297,80,32,true,true,false,0,timeTint);
  std::snprintf(n,sizeof n,".%02u",centis%100);FontText::label(n,FontText::TimeMil,104,304,32,24,true,true,false,0,timeTint);
  const auto mainStroke=std::string(ru?"Удары: ":"Strokes: ")+std::to_string(scene.game.strokes);
  auto strokeText=FontText::characters(mainStroke.c_str(),FontText::Strokes);
  std::string strokeKey=mainStroke+":"+std::to_string(scene.strokeSaturation);
  if(scene.tk_display_active()){std::snprintf(n,sizeof n,".%u",unsigned(scene.tk_fraction()*10));const auto fraction=FontText::characters(n,FontText::TkFraction);strokeText.insert(strokeText.end(),fraction.begin(),fraction.end());strokeKey+=n;}
  if(strokeKey!=FontText::strokeKey){FontText::strokeKey=strokeKey;FontText::strokeTick=scene.tick;}
  const double strokeAge=(scene.tick-FontText::strokeTick)/60.;
  const auto beforeStrokeClip=EditorArt::uiScissor;float strokeHeight=0;const auto strokeLines=FontText::lines(strokeText,168,true);for(const auto&line:strokeLines){const auto m=FontText::metrics(strokeText,line.first,line.last);strokeHeight+=m[0]+m[1];}
  EditorArt::scissor(std::max(beforeStrokeClip[0],ViewX+int(8*EditorArt::Scale)),std::max(beforeStrokeClip[1],int(236*EditorArt::Scale)),std::min(beforeStrokeClip[2],ViewX+int(176*EditorArt::Scale)),std::min(beforeStrokeClip[3],int((236+std::max(32.f,strokeHeight))*EditorArt::Scale)));
  FontText::draw(strokeText,8,236,168,32,false,false,true,0,[&](float pen,unsigned){return FontText::hsv(std::max(scene.strokeSaturation,0.)*std::abs(strokeAge+pen/50),scene.strokeSaturation);});
  EditorArt::scissor(beforeStrokeClip[0],beforeStrokeClip[1],beforeStrokeClip[2],beforeStrokeClip[3]);
  setTexture(GU_PSM_4444,128,64,ParBallTexture);patch({int16_t(32*(scene.tick/6%4)),0,30,30,0,0},87,125);
  const unsigned par=unsigned(std::max(0,scene.par));const int count=std::snprintf(n,sizeof n,"%u",par);const float factor=std::min(1.f,22.f/(12+10*(count-1)));float x=102-(12+10*(count-1))*factor*.5f;
  for(const char*c=n;*c;++c){patch({int16_t(12*(*c-'0')),32,12,18,0,0},x,140-9*factor,factor,factor);x+=10*factor;}
  const auto bonus=std::string(ru?"Бонус Уровня: ":"Level Bonus: ")+std::to_string(scene.bonus);FontText::label(bonus.c_str(),FontText::Bonus,8,260,216,32,false,true,true,3,0xff66faff);
  tv(scene.tick*1000/60,true);EditorRender::clip=savedClip;EditorRender::applyClip();
}
} // namespace EditorPlayHud
