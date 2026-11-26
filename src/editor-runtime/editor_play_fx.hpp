#pragma once
#include "editor_play_hud.hpp"
#include "editor_score_font.hpp"
#include "editor_font_masks.hpp"
#include "live_result_art.hpp"
#include "editor_nuke_art.hpp"
#include <limits>
namespace EditorPlayHud {
struct PopupFX {
  Vec2 position{},scale{1,1};
  Color tint{};
  double rotation=0;
  float alpha=1;
  unsigned age=0;
  char text[64]{};
};
struct NukeFX {Vec2 centers[5]{};unsigned age=0;};
inline std::unique_ptr<PopupFX[]> popups;
inline std::unique_ptr<NukeFX[]> nukes;
inline unsigned popupCount=0,popupCapacity=0,nukeCount=0,nukeCapacity=0;
inline Buffer nukePixels,nukeScratch;
inline std::unique_ptr<EditorNukeArt::Reader> nukeArt;
inline std::array<int,6> nukeSlots{{-1,-1,-1,-1,-1,-1}};
inline unsigned nukeUsed[6]{},nukeClock=0;
inline std::string popupFontPath;
inline std::string popupLiteralPath;
inline EditorImages::Info popupFontInfo{};
inline uint32_t fxRandom=0x053ed17;
inline bool fxReady=false;
inline void clearFX(){popupCount=nukeCount=0;resetTV();FontText::reset();}
inline void unloadFX(){clearFX();popups.reset();nukes.reset();popupCapacity=nukeCapacity=0;nukePixels.clear();nukeScratch.clear();nukeArt.reset();nukeSlots.fill(-1);std::fill_n(nukeUsed,6,0u);nukeClock=0;fxReady=false;popupFontPath.clear();popupLiteralPath.clear();}
inline bool loadFX(const char*exe){
  unloadFX();popupFontPath=companion(exe,EditorRender::ScoreFont::Path);popupLiteralPath=companion(exe,EditorRender::ScoreFont::LiteralPath);
  if(!EditorImages::inspect(popupFontPath.c_str(),popupFontInfo)||popupFontInfo.width!=256||popupFontInfo.height!=256){error="Invalid score font atlas";return false;}
  EditorImages::Info literal;if(!EditorImages::inspect(popupLiteralPath.c_str(),literal)||literal.width!=256||literal.height!=128){error="Invalid source score literal image";return false;}
  nukeArt.reset(new(std::nothrow) EditorNukeArt::Reader);
  if(!nukeArt||!nukePixels.allocate(6*EditorNukeArt::FrameBytes)||!nukeScratch.allocate(65536)||!nukeArt->open(companion(exe,EditorNukeArt::BankPath).c_str())){error="Cannot load lossless original Nuke frames";nukePixels.clear();nukeScratch.clear();nukeArt.reset();return false;}
  fxReady=true;return true;
}
template<class T> inline bool growFX(std::unique_ptr<T[]>&array,unsigned&capacity,unsigned count,unsigned wanted){
  if(wanted<=capacity)return true;
  const unsigned next=std::max(wanted,std::max(64u,capacity<=UINT32_MAX/2?capacity*2:capacity));
  if(next>UINT32_MAX/sizeof(T)){error="Visual effect allocation exceeds address space";return false;}
  std::unique_ptr<T[]> replacement(new(std::nothrow) T[next]);
  if(!replacement){error="Cannot allocate source visual effects";return false;}
  if(count)std::copy_n(array.get(),count,replacement.get());
  array=std::move(replacement);capacity=next;return true;
}
inline float randomFX(){fxRandom^=fxRandom<<13;fxRandom^=fxRandom>>17;fxRandom^=fxRandom<<5;return float(double(fxRandom)/UINT32_MAX);}
// One call per unpaused60Hz step, including terminal animations. The Scene's
// event queue is consumed once by the caller, exactly as for source audio.
inline bool advanceEvents(const EditorPlay::Scene&scene){
  unsigned kept=0;
  for(unsigned i=0;i<popupCount;++i){auto p=popups[i];++p.age;p.alpha=float(double(p.alpha)-.02);if(p.alpha>0)popups[kept++]=p;}popupCount=kept;
  kept=0;for(unsigned i=0;i<nukeCount;++i){auto n=nukes[i];if(++n.age<65)nukes[kept++]=n;}nukeCount=kept;
  for(const auto&e:scene.events){
    if(e.type==EditorPlay::Event::Thought)thought(unsigned(e.value));
    if((e.type==EditorPlay::Event::Score||e.type==EditorPlay::Event::Popup)&&e.popup){
      if(!growFX(popups,popupCapacity,popupCount,popupCount+1))return false;
      auto&p=popups[popupCount++];p=PopupFX{};p.position=e.hasPosition?Vec2{e.position.x,e.position.y}:Vec2{scene.game.position.x,scene.game.position.y};p.scale={e.scale.x,e.scale.y};p.rotation=e.rotation;p.tint=e.tint;
      if(e.text)std::snprintf(p.text,sizeof p.text,"%s",e.text);
      else if(e.score)std::snprintf(p.text,sizeof p.text,"%lld",static_cast<long long>(e.score));
    }
    if(e.type==EditorPlay::Event::Effect&&e.objectId==7&&e.sound&&!std::strcmp(e.sound,"nuke")){
      if(!growFX(nukes,nukeCapacity,nukeCount,nukeCount+1))return false;
      auto&n=nukes[nukeCount++];n=NukeFX{};constexpr Vec2 centers[]={{352,96},{672,96},{512,256},{352,416},{672,416}};
      for(unsigned i=0;i<5;++i)n.centers[i]={centers[i].x+(randomFX()*192-96),centers[i].y+(randomFX()*192-96)};
    }
  }return true;
}
inline const EditorRender::ScoreFont::Glyph* scoreGlyph(unsigned code){for(const auto&g:EditorRender::ScoreFont::Glyphs)if(g.code==code)return &g;return nullptr;}
inline float scoreKerning(unsigned left,unsigned right){for(const auto&k:EditorRender::ScoreFont::Kerning)if(k.left==left&&k.right==right)return k.x;return 0;}
inline void popup(const EditorRender::View&view,const PopupFX&p){
  EditorRender::drawPoof(view,p.position,p.age,p.scale,p.rotation,p.tint);
  if(!*p.text)return;
  const double cs=std::cos(p.rotation),sn=std::sin(p.rotation);Color tint=p.tint;tint.a*=std::clamp(double(p.alpha),0.,1.);
  if(!std::strcmp(p.text,"I don't have a ring.")){
    // Source Label grows both ways beyond its original128px minimum. Use the
    // fixed original TextLine positions, then draw all outlines before fills;
    // fading a precomposited whole word changes overlapping glyph alpha.
    const double origin=-EditorRender::ScoreFont::LiteralWidth*.5;
    for(unsigned layer=0;layer<3;++layer)if(EditorRender::FontMasks::ScoreFlags[0]&(1u<<layer))
      for(const auto& shaped:EditorRender::ScoreFont::LiteralGlyphs){
        const auto*g=scoreGlyph(shaped.code);if(!g)continue;
        const float xs[]={0,float(g->w),float(g->w),0},ys[]={0,0,float(g->h),float(g->h)};std::array<EditorRender::Vertex,4> quad{};
        for(unsigned i=0;i<4;++i){const double x=(origin+shaped.x+g->ox+xs[i])*p.scale.x,y=(7-p.age+g->oy+ys[i])*p.scale.y;quad[i]=view.vertex({p.position.x+x*cs-y*sn,p.position.y+x*sn+y*cs},g->u+xs[i],g->v+ys[i]);}
        EditorRender::FontMasks::draw(EditorRender::FontMasks::Score,0,static_cast<EditorRender::FontMasks::Layer>(layer),quad,EditorRender::rgba(tint),&view,&tint,shaped.x,71);
      }
    return;
  }
  const char*text=p.text;double width=0;unsigned previous=0;
  while(*text){const unsigned code=EditorArt::codepoint(text);const auto*g=scoreGlyph(code);if(!g)continue;width+=g->advance+scoreKerning(previous,code);previous=code;}
  const double size=std::ceil(width),origin=size<=128?std::floor((128-size)*.5)-64:-size*.5;
  for(unsigned layer=0;layer<3;++layer)if(EditorRender::FontMasks::ScoreFlags[0]&(1u<<layer)){
    double pen=0;previous=0;text=p.text;
    while(*text){const unsigned code=EditorArt::codepoint(text);const auto*g=scoreGlyph(code);if(!g)continue;pen+=scoreKerning(previous,code);previous=code;
      const float xs[]={0,float(g->w),float(g->w),0},ys[]={0,0,float(g->h),float(g->h)};
      std::array<EditorRender::Vertex,4> quad{};
      for(unsigned i=0;i<4;++i){const double x=(origin+pen+g->ox+xs[i])*p.scale.x,y=(7-p.age+g->oy+ys[i])*p.scale.y;quad[i]=view.vertex({p.position.x+x*cs-y*sn,p.position.y+x*sn+y*cs},g->u+xs[i],g->v+ys[i]);}
      EditorRender::FontMasks::draw(EditorRender::FontMasks::Score,0,static_cast<EditorRender::FontMasks::Layer>(layer),quad,EditorRender::rgba(tint),&view,&tint,float(pen),71);
      pen+=g->advance;
    }
  }
}
inline void drawWorldFX(const EditorController::Controller&controller,const EditorPlay::Scene&scene){
  if(!fxReady)return;
  EditorRender::View view(controller,&scene);EditorArt::scissor(ViewX+136,0,ViewX+408,272);
  for(unsigned i=0;i<popupCount;++i)popup(view,popups[i]);
  EditorArt::fullView();
}
inline const void* nukeFrame(unsigned frame){
  if(!nukeArt||frame>=20)return nullptr;
  ++nukeClock;unsigned slot=0;for(unsigned i=0;i<6;++i){if(nukeSlots[i]==int(frame)){nukeUsed[i]=nukeClock;return nukePixels.data+i*EditorNukeArt::FrameBytes;}if(nukeUsed[i]<nukeUsed[slot])slot=i;}
  // A cache page can still be referenced by the current GE list. Complete it
  // before replacing that page, including overlapping simultaneous Nukes.
  flushBatch();beginBatch();EditorArt::fullView();
  // A failed inflate can overwrite output: retire its previous tag first.
  nukeSlots[slot]=-1;
  auto* pixels=nukePixels.data+slot*EditorNukeArt::FrameBytes;
  if(!nukeArt->decode(frame,pixels,EditorNukeArt::FrameBytes,nukeScratch.data,65536)){error="Cannot decode original Nuke frame";return nullptr;}
  sceKernelDcacheWritebackRange(pixels,EditorNukeArt::FrameBytes);
  nukeSlots[slot]=int(frame);nukeUsed[slot]=nukeClock;return pixels;
}
inline void drawOverlayFX(){
  if(!fxReady||!nukeCount)return;
  EditorArt::fullView();sceGuBlendFunc(GU_ADD,GU_SRC_ALPHA,GU_ONE_MINUS_SRC_ALPHA,0,0);constexpr unsigned frames[]={0,1,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19};
  for(unsigned n=0;n<nukeCount;++n)for(unsigned i=0;i<5;++i){const unsigned delay=3*i;if(nukes[n].age<delay)continue;const unsigned at=(nukes[n].age-delay)*25/60;if(at>=std::size(frames))continue;
    if(const auto*p=nukeFrame(frames[at])){sceGuClutMode(GU_PSM_8888,0,255,0);sceGuClutLoad(32,static_cast<const uint8_t*>(p)+65536);setTexture(GU_PSM_T8,256,256,p);sceGuTexFilter(GU_NEAREST,GU_NEAREST);sceGuTexFlush();patch({0,0,256,256,0,0},float((nukes[n].centers[i].x-128)*EditorArt::Scale),float((nukes[n].centers[i].y-128)*EditorArt::Scale),EditorArt::Scale,EditorArt::Scale,0x5affffff);}
  }
}
inline void drawFX(const EditorController::Controller&controller,const EditorPlay::Scene&scene){drawWorldFX(controller,scene);drawOverlayFX();}
} // namespace EditorPlayHud
