#pragma once
#include "editor_play_hud.hpp"
#include "editor_audio.hpp"
#include "editor_result_state.hpp"
#include "results.hpp"
#include "live_result_art.hpp"
#include "live_result_slide.inc"
#include "pause_menu.hpp"
#include "editor_hud_assets.hpp"
namespace EditorPlayHud {
inline std::unique_ptr<ResultAssets> result;
inline std::unique_ptr<RankPlayer> rankArt;
inline std::unique_ptr<LiveResultArt> winArt,loseArt;
inline LiveResultArt::Slot largeSlot,clearSlot,confettiSlot;
inline Buffer resultPixels,resultScratch;
inline Buffer skipPixels;
alignas(64) inline uint32_t skipPalette[256]{};
inline bool stateReady=false;
inline void unloadState(){rankArt.reset();result.reset();winArt.reset();loseArt.reset();resultPixels.clear();resultScratch.clear();skipPixels.clear();largeSlot={};clearSlot={};confettiSlot={};stateReady=false;}
inline bool loadState(const char*exe){
  unloadState();
  result.reset(new(std::nothrow) ResultAssets);rankArt.reset(new(std::nothrow) RankPlayer);winArt.reset(new(std::nothrow) LiveResultArt);loseArt.reset(new(std::nothrow) LiveResultArt);
  const auto fail=[](const char*why){error=why;unloadState();return false;};
  if(!result||!rankArt||!winArt||!loseArt||!resultPixels.allocate(ResultVramBytes)||!resultScratch.allocate(512*272*2))return fail("Result page allocation failed");
  const auto localized=[&](const char*name){return companion(exe,(std::string("data/locale/")+(ru?"ru/":"en/")+name).c_str());};
  if(!result->load(localized("results.pak").c_str())||!rankArt->open(localized("ranks.pak").c_str())||!winArt->open(companion(exe,"data/interface/results-win.ucr"),62,512*272*2)||!loseArt->open(companion(exe,"data/interface/results-native.ucr"),123,512*272*2)||!PauseArt::load(companion(exe,"data/interface/pause-menu.bin").c_str()))return fail("Cannot read authored result/pause art");
  const auto profiles=openAssetFile(companion(exe,"data/interface/profiles-art.pak").c_str());uint32_t magic=0;
  // UCF4 has17 profile patches perlocale; only the independent SkipHUD slice is loaded.
  constexpr unsigned skipOffset=4+2*17*sizeof(Patch)+424*512*2;
  if(!skipPixels.allocate(512*128)||!read(profiles.get(),0,&magic,4)||magic!=0x34464355||!read(profiles.get(),skipOffset,skipPalette,sizeof skipPalette)||!read(profiles.get(),skipOffset+1024,skipPixels.data,512*128))return fail("Cannot read authored EN/RU skipping HUD");
  sceKernelDcacheWritebackRange(skipPalette,sizeof skipPalette);sceKernelDcacheWritebackRange(skipPixels.data,512*128);
  stateReady=true;return true;
}
inline float slide(unsigned elapsed){const unsigned t=std::min(elapsed,2000u)*60,i=t/1000;if(i>=std::size(ResultSlide040)-1)return 0;return ResultSlide040[i]+(ResultSlide040[i+1]-ResultSlide040[i])*(t%1000)/1000.f;}
inline unsigned extent(unsigned n){unsigned p=1;while(p<n)p*=2;return p;}
inline void resultSprite(const LiveResultArt::Slot&slot,float y=0){
  const auto&s=slot.sprite;
  for(unsigned i=0;i<s.count;++i){const auto&t=slot.images[i].tile;
    sceGuEnable(GU_TEXTURE_2D);sceGuTexMode(t.format?GU_PSM_5650:GU_PSM_8888,0,0,1);sceGuTexImage(0,extent(t.width),extent(t.height),t.pitch,slot.images[i].pixels);sceGuTexFlush();
    patch({0,0,int16_t(t.width),int16_t(t.height),0,0},s.x+t.x,s.y+t.y+y);
  }
}
inline void resultLabel(const std::string&text,float x,float y,bool small=false,uint32_t tint=White,bool centered=false){
  if(ru||small){localeText(text.c_str(),x,y,.94f,tint,centered);return;}
  unsigned width=0;if(centered)for(unsigned char c:text)if(c>=32&&c<128)width+=result->advance[0][c-32];
  x-=width/16.f;setTexture(GU_PSM_8888,256,256,result->letters);
  for(unsigned char c:text){if(c<32||c>=128)continue;const unsigned at=c-32;patch({int16_t((at%16)*16),int16_t((at/16)*16),16,16,0,0},x,y,1,1,tint);x+=result->advance[0][at]/8.f;}
}
inline unsigned rank(const EditorPlay::Scene&s){unsigned r=0;while(r<5&&s.final_score()>=s.rankReqs[r+1])++r;return r;}
inline int64_t displayedScore(const EditorPlay::Scene&s,unsigned ms){const auto total=s.final_score();if(ms>=3890)return total;if(ms<3050)return 0;return int64_t(total*(1-std::exp2(-8*(ms-3050)/840.))/(1-1./256));}
// Call once after State::update. Callback arguments: bank id, gain dB, pitch.
inline void sounds(const EditorPlay::Scene&s,State&state,void(*play)(unsigned,float,float)){
  if(!play)return;
  if(state.mode!=State::Mode::Won){state.heardRank=0;return;}
  for(unsigned i=0;i<3;++i)if(state.soundMask&(1u<<i)){play(319,0,1);play(321,0,1+i*.1f);}
  if(state.soundMask&8)play(320,0,1);
  const auto score=displayedScore(s,state.elapsed);
  if(state.elapsed>=3050)while(state.heardRank<5&&score>=s.rankReqs[state.heardRank+1]){state.prismStarted[state.heardRank]=s.tick;++state.heardRank;play(ucg::editor_audio::CollectPrism,0,1.75f);}
  if(state.soundMask&16){constexpr unsigned ids[]={329,314,315,316,318,317};const unsigned r=rank(s);play(ids[r],r?0:-4.981f,1);}
}
inline void resultStats(const EditorPlay::Scene&s,const State&state,unsigned ms,float movement){
  constexpr uint32_t green=0xff00ff00,red=0xff0033ff;char text[160];const int64_t diff=int64_t(s.game.strokes)-s.par;
  const auto strokeBonus=std::max<int64_t>(5000+500*(int64_t(s.par)-s.game.strokes),0);
  const auto timeBonus=std::max<int64_t>(EditorPlay::source_integer(2500-100*(s.elapsedTicks/60.-s.onTime)),0);
  if(ms>=2010){
    if(s.game.strokes<=1)FontText::label(s.game.strokes?lang("HOLE IN ONE!","ДЫРКА В ОДНОМ!"):lang("HOLE IN NONE!","ДЫРКА В НУЛИ?"),FontText::Result16,288,160,192,36,false,true);
    else{std::snprintf(text,sizeof text,lang("Strokes: %u","Удары: %u"),s.game.strokes);FontText::label(text,FontText::Result16,288,160,192,36,false,true);}
    std::string detail;
    if(!diff)detail=lang("Even with Par!","Равен с Паром!");
    else{std::snprintf(text,sizeof text,diff>0?lang("%u Above Par...","%u Выше пар..."):lang("%u Below Par!","%u Ниже пар!"),unsigned(std::abs(diff)));detail=text;}
    detail+="\n"+std::string(lang("Bonus: ","Бонус: "))+std::to_string(strokeBonus);
    FontText::label(detail.c_str(),FontText::Result12,288,196,192,74,false,false,false,-6,diff>0?red:green);
  }
  if(ms>=2355){
    const unsigned c=s.elapsedTicks*5/3;std::snprintf(text,sizeof text,lang("Time: %02u:%02u.%02u","Время: %02u:%02u.%02u"),c/6000,c/100%60,c%100);
    FontText::label(text,FontText::Result16,288,264,192,36,false,true);
    const double late=s.elapsedTicks/60.-s.onTime;const uint32_t tint=late<0?green:late<1?0xff00ffff:red;
    std::string detail;if(late<0)detail=lang("On Time!","Во Время!");else if(late<1)detail=lang("Just Barely Late...","Капельку поздно..");
    else{std::snprintf(text,sizeof text,lang("%u Seconds Late...","%u Секунд поздно..."),unsigned(late));detail=text;}
    detail+="\n"+std::string(lang("Bonus: ","Бонус: "))+std::to_string(timeBonus);
    FontText::label(detail.c_str(),FontText::Result12,288,300,192,74,false,false,false,-6,tint);
  }
  if(ms>=2705)FontText::label((std::string(lang("Level Bonus: ","Бонус Уровня: "))+std::to_string(s.bonus)).c_str(),FontText::Result16,288,368,192,36,false,true);
  const auto total=s.final_score(),displayed=displayedScore(s,ms);
  setTexture(GU_PSM_8888,256,64,result->bar);patch({0,0,204,9,0,0},170,212+movement);
  const double peak=std::max(1,s.rankReqs[5]);const int16_t filled=int16_t(204*std::clamp(double(displayed)/peak,0.,1.));if(filled)patch({0,16,filled,9,0,0},170,212+movement);
  for(unsigned i=1;i<6;++i){const double fraction=s.rankReqs[i]/peak;if(!std::isfinite(fraction))continue;
    float jump=0;if(i<=state.heardRank){jump=-12;const unsigned age=s.tick-state.prismStarted[i-1];for(unsigned step=0;step<std::min(age,512u);++step)jump+=(0-jump)*.15f;}
    const auto&image=i<=state.heardRank?EditorRender::HudAssets::prism:EditorRender::HudAssets::prism_black;
    const unsigned frame=unsigned(s.tick/3.+.870705)%10;const EditorArt::Rect region{float(frame*32),0,32,32};const EditorImages::Info info{image.width,image.height,false};
    const auto saved=EditorRender::clip;EditorRender::clip={float(ViewX),0,float(ViewX+408),272};
    EditorRender::customImage(hudRoot+image.path,info,region,[&](Vec2 pixel){return EditorRender::Vertex{ViewX+float(320+fraction*384+pixel.x-region.x-16)*EditorArt::Scale,float(407+jump+pixel.y-16)*EditorArt::Scale+movement,0,0};},White);
    EditorRender::clip=saved;EditorRender::applyClip();
  }
  FontText::label((std::string(lang("FINAL SCORE: ","Результат: "))+std::to_string(displayed)).c_str(),FontText::Result16,320,416+movement/EditorArt::Scale,384,32,true,true);
  if(ms>=4400){
    const unsigned r=rank(s);RankPlayer::Frame f{};if(const auto*p=rankArt->frame(r,ms-4400,f)){setTexture(GU_PSM_8888,128,128,p);patch({0,0,f.width,f.height,0,0},323-f.width/2,144.5f-f.height/2);}
    if(r<5)FontText::label((std::to_string(int64_t(s.rankReqs[r+1])-total)+" to Next Rank").c_str(),FontText::Result10,320,464,384,32,true,true,false,0,0xffc4c4c4);
  }
}
inline void pause(unsigned elapsed,unsigned choice,bool held,bool testing=false){
  using namespace PauseArt;
  const auto layer=[](const uint8_t*p){sceGuEnable(GU_TEXTURE_2D);sceGuTexMode(GU_PSM_T8,0,0,1);sceGuClutMode(GU_PSM_8888,0,255,0);sceGuClutLoad(32,colors);sceGuTexImage(0,512,512,Field,p);patch({0,0,Field,Field,0,0},136,0);};
  layer(darkness);setTexture(GU_PSM_8888,PauseArt::Width,PauseArt::Height,atlas);const auto&head=(ru?RuHeadline:EnHeadline)[0];patch(head,head.x,head.y);
  for(unsigned i=0;i<(testing?2u:3u);++i){const Patch*labels=ru?RuNormal:EnNormal;if(i==choice)labels=held?(ru?RuPressed:EnPressed):(ru?RuHover:EnHover);const auto&p=labels[i];patch(p,p.x,p.y);}
  for(const auto&p:{Dancer[elapsed/200%4],(ru?RuPrompt:EnPrompt)[0]})patch(p,p.x,p.y);
  layer(filterPixels(elapsed*3/100%FilterFrames));
}
inline void drawSkipping(unsigned ticks){
  if(!stateReady||!ticks)return;
  EditorArt::fullView();constexpr float scale=17.f/32;
  rectangle(ViewX+256*scale,504*scale,512*scale*std::min(ticks/30.f,1.f),8*scale,0xa5ffff00);
  const unsigned age=ticks-1,frame=std::min(age/10%4,2u),index=unsigned(ru)*6+frame;
  sceGuClutMode(GU_PSM_8888,0,255,0);sceGuClutLoad(32,skipPalette);
  sceGuBlendFunc(GU_ADD,GU_FIX,GU_ONE_MINUS_SRC_ALPHA,0x00ffffff,0);
  setTexture(GU_PSM_T8,512,128,skipPixels.data);patch({int16_t(128*(index%4)),int16_t(40*(index/4)),108,34,0,0},300,238);
  sceGuBlendFunc(GU_ADD,GU_SRC_ALPHA,GU_ONE_MINUS_SRC_ALPHA,0,0);
}
inline void hudImage(const EditorRender::HudAssets::Image&image,EditorArt::Rect region,float centerX,float centerY,float sx=1,float sy=1,uint32_t tint=White,bool gradient=false,const Color*sourceTint=nullptr){
  if(!image.width||!image.height||!*image.path)return;
  const EditorImages::Info info{image.width,image.height,false};
  const auto transform=[&](Vec2 pixel){return EditorRender::Vertex{ViewX+(centerX+(float(pixel.x)-region.x-region.w*.5f)*sx)*EditorArt::Scale,(centerY+(float(pixel.y)-region.y-region.h*.5f)*sy)*EditorArt::Scale,0,0};};
  const auto draw=[&](uint32_t color){EditorRender::customImage(hudRoot+image.path,info,region,transform,color,nullptr,gradient?nullptr:sourceTint);};
  if(gradient)rainbow(ViewX+(centerX-region.w*.5f*sx)*EditorArt::Scale,ViewX+(centerX+region.w*.5f*sx)*EditorArt::Scale,draw);else draw(tint);
}
inline void death(const EditorPlay::Scene&s,unsigned elapsed){
  using namespace EditorRender::HudAssets;
  const auto before=EditorRender::clip;EditorRender::clip={float(ViewX),0,float(ViewX+408),272};EditorArt::fullView();
  if(s.deathReason==1){const bool second=(elapsed/200)%2;hudImage(out_of_bounds,second?EditorArt::Rect{225,0,25,10}:EditorArt::Rect{0,0,254,108},512,256,.75f,.75f);}
  else if(s.deathReason==2){
    hudImage(backrooms,{0,0,float(backrooms.width),float(backrooms.height)},512,256);
    const auto&body=Sprites[pair.player];hudImage(body,{0,0,float(body.width),float(body.height)},624,400,.125f,.125f,0xffa6f9ff,costume!=VillaCostumes::Id::Copycat&&VillaCostumes::Art::rainbow(pair.player));
    if(hasInk){sceGuClutMode(GU_PSM_8888,0,255,0);sceGuClutLoad(3,ArtistInk::Palette);setTexture(GU_PSM_T8,128,128,ink.data);patch(ArtistInk::Full,624*EditorArt::Scale-32*EditorArt::Scale,400*EditorArt::Scale-32*EditorArt::Scale,.5f*EditorArt::Scale,.5f*EditorArt::Scale,0xffa6f9ff);}
  }
  else if(s.deathReason==4)hudImage(costume_horse,{0,0,float(costume_horse.width),float(costume_horse.height)},512,256);
  else if(s.deathReason!=3&&s.deathReason!=5){const auto&ghost=Sprites[pair.ghost];hudImage(ghost,{0,0,float(ghost.width),float(ghost.height)},512,256,1,1,EditorRender::rgba(s.deathTint),VillaCostumes::Art::rainbow(pair.ghost),&s.deathTint);}
  EditorRender::clip=before;EditorArt::fullView();
}
inline void drawState(const EditorPlay::Scene&s,const State&state,bool testing=false){
  if(!stateReady)return;
  EditorArt::fullView();
  if(state.mode==State::Mode::Paused){pause(state.elapsed,state.choice,state.armed,testing);return;}
  if(state.mode==State::Mode::Dead){death(s,state.elapsed);return;}
  if(state.mode!=State::Mode::Dead&&state.mode!=State::Mode::Won)return;
  const bool won=state.mode==State::Mode::Won;auto&art=won?*winArt:*loseArt;auto*p=resultPixels.data;auto*scratch=resultScratch.data;
  if(!art.prepare(won?0:3,largeSlot,p,ResultLargeBytes,scratch,512*272*2,true)){error="Cannot decode result panel";return;}
  const float movement=won?slide(state.elapsed):0;
  EditorArt::scissor(ViewX+136,0,ViewX+408,272);resultSprite(largeSlot,movement);
  if(won){
    if(art.prepare(ru?2:1,clearSlot,p+ResultLargeBytes,ResultClearBytes,scratch,512*272*2,true))resultSprite(clearSlot,movement);
    if(art.prepare(3+(state.elapsed/40)%59,confettiSlot,p+ResultLargeBytes+ResultClearBytes,ResultConfettiBytes,scratch,512*272*2,true))resultSprite(confettiSlot);
    resultStats(s,state,state.elapsed,movement);
  }
  EditorArt::fullView();
}
} // namespace EditorPlayHud
