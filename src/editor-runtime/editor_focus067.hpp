#pragma once
#include "ui_modality067.hpp"
#include "editor_list_layout067.hpp"
#include "editor_modifier_help067.hpp"
#include "editor_hotkey_help068.hpp"
#include <array>
#include <vector>
namespace EditorFocus067 {
using Target=UiModality067::Target;using Rect=UiModality067::Rect;
struct Targets {
 std::array<Target,512> entries{};unsigned count=0;bool overflow=false;
 void clear(){count=0;overflow=false;}
 void push_back(Target t){if(count<entries.size())entries[count++]=t;else overflow=true;}
 Target*begin(){return entries.data();}Target*end(){return entries.data()+count;}
 const Target*begin()const{return entries.data();}const Target*end()const{return entries.data()+count;}
};
inline void add(Targets&v,int id,float x,float y,float w,float h){if(w>0&&h>0)v.push_back({id,{x,y,w,h},true});}
template<class R>inline void add(Targets&v,int id,const R&r,float x=0,float y=0){add(v,id,r.x+x,r.y+y,r.w,r.h);}
inline void outline(const Rect&r){
 const float x=std::max(0.f,r.x),y=std::max(0.f,r.y),right=std::min(768.f,r.x+r.w),bottom=std::min(512.f,r.y+r.h);
 if(x>=right||y>=bottom)return;
 EditorArt::fullView();
 EditorPanel::box(x,y,right-x,2,White);EditorPanel::box(x,bottom-2,right-x,2,White);EditorPanel::box(x,y,2,bottom-y,White);EditorPanel::box(right-2,y,2,bottom-y,White);
}
struct State {UiModality067::Focus focus;Targets targets;Rect highlighted{};bool valid=false;
 unsigned fileFocusGeneration=~0u,fileSelected=~0u;std::array<unsigned,6>scrollRevision{};
 void select(unsigned context,unsigned pressed,int preferred=-1){
  if(context!=focus.context){focus.context=context;focus.id=preferred;}
  focus.step(targets,context,pressed);const auto*t=focus.selected(targets);valid=t;highlighted=t?t->rect:Rect{};
 }
 ucg::editor::Vec2 point(ucg::editor::Vec2 original)const{return valid?ucg::editor::Vec2{highlighted.x+highlighted.w*.5f,highlighted.y+highlighted.h*.5f}:original;}
 void draw()const{if(valid)outline(highlighted);}
};
}
namespace EditorPanel {
// Existing friend declaration grants access without changing Panel fields or its
// mouse handlers. Stable semantic IDs are separate from the analog pointer.
struct PanelTestAccess {
 using State=EditorFocus067::State;using Target=EditorFocus067::Target;
 static bool precise(const Panel&p){return p.c.dragging||p.colorSampling||colorCapture(p);}
 static bool colorCapture(const Panel&p){return p.colorUi&&((p.colorUi->dragKind>=0)||p.presetDrag>=0||p.spinChannel>=0);}
 static bool pointerRequired(const Panel&p){return p.colorSampling;}
 static bool visible(const Panel&p,unsigned i){const auto&c=EditorArt::Packed::Controls[i];const char*path=c.path;
  if(!std::strncmp(path,"pickOption/",11))return false;
  if(!std::strncmp(path,"HUD/Tools/",10))return p.c.tools;
  if(c.tab>=0&&c.tab!=int(p.c.tab))return false;
  return !std::strstr(path,"example");
 }
 static unsigned context(const Panel&p){return p.keyboard.active()?1:p.overwrite?2:p.exitQuestion?3:p.files?4:p.colorOpen?(p.colorMenu>=0?6:5):p.popup>=0?7:p.welcome?8:16+p.c.tab;}
 static void menu(EditorFocus067::Targets&v,int base,const EditorMenuArt::Layout&g,const std::vector<EditorMenuArt::Item>&items,float x,float y){
  // Existing layout owns row metadata; binary-search the first visible row and
  // include one neighbor at each end for logical scrolling without a full scan.
  size_t lo=0,hi=g.rows.size();while(lo<hi){const auto m=(lo+hi)/2;const auto&r=g.rows[m];if(r.hit.y+r.hit.h<g.content.y)lo=m+1;else hi=m;}
  if(lo){do{--lo;const auto i=g.rows[lo].index;if(i<items.size()&&!items[i].disabled&&!items[i].separator)break;}while(lo);}
  for(size_t n=lo;n<g.rows.size();++n){const auto&r=g.rows[n];const bool enabled=r.index<items.size()&&!items[r.index].disabled&&!items[r.index].separator;if(enabled)EditorFocus067::add(v,base+int(r.index),r.hit,x,y);if(enabled&&r.hit.y>g.content.y+g.content.h)break;}
 }
 static void gather(Panel&p,State&s){
  using EditorFocus067::add;s.targets.clear();auto&v=s.targets;
  if(p.keyboard.active())return;
  if(p.overwrite||p.exitQuestion){add(v,80000,320,256,104,32);add(v,80001,448,256,104,32);return;}
  if(p.files){
   add(v,51000,284,82,452,44);
   const unsigned first=p.fileScroll?p.fileScroll-1:0,last=std::min<unsigned>(p.fileEntries.size(),p.fileScroll+EditorTextLayout068::FileRows+1);
   for(unsigned i=first;i<last;++i)add(v,2000000+int(i),284,EditorTextLayout068::FileTop+(int(i)-int(p.fileScroll))*EditorTextLayout068::FilePitch,452,EditorTextLayout068::FilePitch);
   add(v,51001,284,370,452,44);add(v,51002,284,426,128,40);add(v,51003,428,426,128,40);add(v,51004,572,426,164,40);return;
  }
  if(p.colorOpen&&p.colorUi){
   if(p.colorSampling)return;
   if(p.colorMenu>=0){menu(v,60000,p.colorMenuGeometry,p.colorMenuItems,p.colorMenuX,p.colorMenuY);return;}
   const auto m=p.colorUi->layout();
   auto target=[&](int id,const EditorColorPickerArt::Rect&r){if(r.w>0&&r.h>0)add(v,id,EditorColorPickerArt::logicalRect068(r,p.colorX,p.colorY));};
   target(61000,m.sampler);auto sample=m.sample;sample.w*=.5f;target(61001,sample);target(61002,m.shape);
   for(unsigned i=0;i<3;++i){target(61010+int(i),m.mode[i]);}target(61013,m.modeMenu);
   for(unsigned i=0;i<4;++i)target(61020+int(i),m.spinText[i]);
   if(p.colorUi->hexVisible())target(61024,m.hex);
   target(61025,m.swatchesHeader);target(61026,m.paletteMenu);target(61027,m.recentHeader);
   if(p.colorUi->savedOpen){target(61028,m.add);target(61029,m.presetPrevious068);target(61030,m.presetNext068);
    const unsigned first=m.presetFirstRow068>1?(m.presetFirstRow068-1)*EditorColorPickerArt::Columns-1:0,last=std::min(m.presetCount,(m.presetFirstRow068+m.presetRows068+1)*EditorColorPickerArt::Columns-1);
    for(unsigned i=first;i<last;++i)target(3000000+int(i),m.presetUnclipped068(i));
   }
   if(p.colorUi->recentOpen)for(unsigned i=0;i<m.recentCount;++i)target(4000000+int(i),m.recent(i));
   return;
  }
  if(p.popup>=0){menu(v,40000,p.popupGeometry,p.popupItems,p.popupX,p.popupY);return;}
  if(p.welcome){add(v,70000,EditorArt::controlRect(unsigned(control("pickOption/create"))));add(v,70001,EditorArt::controlRect(unsigned(control("pickOption/edit"))));return;}
  for(unsigned i=0;i<std::size(EditorArt::Packed::Controls);++i)if(visible(p,i)){
   const auto&c=EditorArt::Packed::Controls[i];if(!std::strcmp(c.type,"ItemList"))continue;
   add(v,int(i),EditorArt::controlRect(i));
  }
  if(p.c.tab==4){namespace Help=EditorHotkeyHelp068;const unsigned first=p.hotkeyHelpScroll?p.hotkeyHelpScroll-1:0,last=std::min(Help::Count,p.hotkeyHelpScroll+Help::Rows+1);for(unsigned i=first;i<last;++i)add(v,Help::FocusBase+int(i),Help::X,Help::rowY(i,p.hotkeyHelpScroll),Help::Width,Help::Row);}
  if(p.c.tab==5){namespace Help=EditorModifierHelp067;const unsigned first=p.modifierHelpScroll?p.modifierHelpScroll-1:0,last=std::min(Help::Count,p.modifierHelpScroll+Help::Rows+1);for(unsigned i=first;i<last;++i)add(v,Help::FocusBase+int(i),Help::X,Help::rowY(i,p.modifierHelpScroll),Help::Width,Help::Row);}
  if(p.c.tab==0){const unsigned first=p.floorScroll?p.floorScroll-1:0,last=std::min<unsigned>(std::size(EditorArt::Packed::Floors),p.floorScroll+EditorTextLayout068::FloorRows+1);for(unsigned i=first;i<last;++i)add(v,3000+int(i),20,EditorTextLayout068::y(0,EditorTextLayout068::FloorTop+(int(i)-int(p.floorScroll))*EditorTextLayout068::FloorPitch),216,EditorTextLayout068::FloorPitch);}
  if(p.c.tab==2){
   const unsigned first=p.objectScroll?p.objectScroll-1:0,last=std::min(26u,p.objectScroll+EditorListLayout067::ObjectRows+1);
   for(unsigned i=first;i<last;++i)add(v,1000+int(i),28,100+float(int(i)-int(p.objectScroll))*EditorListLayout067::ObjectRow,200,EditorListLayout067::ObjectRow);
   if(const auto*def=p.syncPropertyLayout()){
    const unsigned count=unsigned(def->variable_count);
    for(unsigned i=p.propertyScroll;i<count&&i<p.propertyScroll+EditorPropertyLayout067::Rows;++i)add(v,EditorPropertyLayout067::FocusBase+int(i),EditorPropertyLayout067::hit(i,p.propertyScroll));
    add(v,EditorPropertyLayout067::ColorFocus,EditorPropertyLayout067::Color);
    if(p.propertyScroll)add(v,EditorPropertyLayout067::PreviousFocus,EditorPropertyLayout067::Previous);
    if(p.propertyScroll<EditorPropertyLayout067::lastScroll(count))add(v,EditorPropertyLayout067::NextFocus,EditorPropertyLayout067::Next);
   }
  }
  if(p.c.tab==3){const unsigned first=p.decoScroll?p.decoScroll-1:0,last=std::min<unsigned>(66+p.c.customDeco.size(),p.decoScroll+EditorListLayout067::DecoRows+1);for(unsigned i=first;i<last;++i)add(v,1000000+int(i),28,EditorTextLayout068::y(3,108)+float(int(i)-int(p.decoScroll))*EditorListLayout067::DecoRow,200,EditorListLayout067::DecoRow);}
  if(p.c.tab==1)for(unsigned i=0;i<std::size(EditorArt::Packed::PickerCells);++i){const auto&c=EditorArt::Packed::PickerCells[i];const float x=19+c.x*16,y=40+c.y*16;if(x>=16&&x<240&&y>=144&&y<352)add(v,20000+int(i),x,y,16,16);}
 }
 static bool ensureMenu(const EditorMenuArt::Layout&g,int row,float&scroll){
  const float old=scroll;size_t lo=0,hi=g.rows.size();while(lo<hi){const auto m=(lo+hi)/2;if(int(g.rows[m].index)<row)lo=m+1;else hi=m;}
  if(lo<g.rows.size()&&int(g.rows[lo].index)==row){const auto&r=g.rows[lo];if(r.hit.y<g.content.y)scroll+=r.hit.y-g.content.y;else if(r.hit.y+r.hit.h>g.content.y+g.content.h)scroll+=r.hit.y+r.hit.h-g.content.y-g.content.h;scroll=std::clamp(scroll,0.f,g.scrollMax);}
  return scroll!=old;
 }
 static void ensure(Panel&p,int id){
  if(p.colorOpen&&p.colorUi&&id>=3000000&&id<4000000){const unsigned index=unsigned(id-3000000);p.colorUi->presetScroll068=std::min(EditorColorPickerArt::ensurePreset068(index,p.colorUi->presetScroll068),EditorColorPickerArt::lastPresetRow068(unsigned(p.colorUi->visual.size())));return;}

  if(id>=1&&id<=46){const auto r=EditorArt::controlRect(unsigned(id));EditorTextLayout068::ensure(p.c.tab,r.y,r.h);}
  else if(id>=1000000&&id<2000000){const float y=EditorTextLayout068::y(3,108)+(id-1000000-int(p.decoScroll))*EditorListLayout067::DecoRow;EditorTextLayout068::ensure(3,y,EditorListLayout067::DecoRow);}
  else if(id>=3000&&id<4000){const float y=EditorTextLayout068::y(0,EditorTextLayout068::FloorTop+(id-3000-int(p.floorScroll))*EditorTextLayout068::FloorPitch);EditorTextLayout068::ensure(0,y,EditorTextLayout068::FloorPitch);}
  if(id>=EditorHotkeyHelp068::FocusBase&&id<EditorHotkeyHelp068::FocusBase+int(EditorHotkeyHelp068::Count))p.hotkeyHelpScroll=EditorHotkeyHelp068::ensureVisible(unsigned(id-EditorHotkeyHelp068::FocusBase),p.hotkeyHelpScroll);
  else if(id>=EditorModifierHelp067::FocusBase&&id<EditorModifierHelp067::FocusBase+int(EditorModifierHelp067::Count))p.modifierHelpScroll=EditorModifierHelp067::ensureVisible(unsigned(id-EditorModifierHelp067::FocusBase),p.modifierHelpScroll);
  else if(id>=40000&&id<50000&&p.popup>=0){if(ensureMenu(p.popupGeometry,id-40000,p.popupScroll))p.rebuildPopup();}
  else if(id>=60000&&id<61000&&p.colorMenu>=0){float scroll=p.colorMenuGeometry.scroll;if(ensureMenu(p.colorMenuGeometry,id-60000,scroll))p.colorMenuGeometry=EditorMenuArt::layout(p.colorMenuItems,0,760,504,scroll);}
  else if(p.files&&id>=2000000&&id<3000000){const unsigned n=unsigned(id-2000000);p.fileSelected=n;if(n<p.fileScroll)p.fileScroll=n;if(n>=p.fileScroll+EditorTextLayout068::FileRows)p.fileScroll=n-EditorTextLayout068::FileRows+1;}
  else if(id>=EditorPropertyLayout067::FocusBase&&id<EditorPropertyLayout067::ColorFocus){if(const auto*def=p.syncPropertyLayout())p.propertyScroll=EditorPropertyLayout067::ensureVisible(unsigned(id-EditorPropertyLayout067::FocusBase),p.propertyScroll,unsigned(def->variable_count));}
  else if(id>=1000&&id<1026){const unsigned n=unsigned(id-1000);if(n<p.objectScroll)p.objectScroll=n;if(n>=p.objectScroll+EditorListLayout067::ObjectRows)p.objectScroll=n-EditorListLayout067::ObjectRows+1;}
  else if(id>=1000000&&id<2000000){const unsigned n=unsigned(id-1000000);if(n<p.decoScroll)p.decoScroll=n;if(n>=p.decoScroll+EditorListLayout067::DecoRows)p.decoScroll=n-EditorListLayout067::DecoRows+1;}
  else if(id>=3000&&id<4000){const unsigned n=unsigned(id-3000);if(n<p.floorScroll)p.floorScroll=n;if(n>=p.floorScroll+EditorTextLayout068::FloorRows)p.floorScroll=n-EditorTextLayout068::FloorRows+1;}
 }
 static void prepare(Panel&p,State&s,unsigned pressed){
  const auto ctx=context(p);
  if(ctx==16+p.c.tab&&p.c.tab<6&&s.scrollRevision[p.c.tab]!=EditorTextLayout068::revision[p.c.tab]){s.focus.context=ctx;s.focus.id=0;s.scrollRevision[p.c.tab]=EditorTextLayout068::revision[p.c.tab];}
  if(ctx==4&&(s.focus.context!=ctx||s.fileFocusGeneration!=p.fileFocusGeneration||s.fileSelected!=p.fileSelected)){
   // A row ID belongs to one listing. Never carry it into a new directory,
   // reopened dialog or externally selected (pointer/keyboard) file row.
   s.focus.context=ctx;s.focus.id=p.fileEntries.empty()?-1:2000000+int(p.fileSelected);
  }
  // Within properties Up/Down follows source variable order, including rows
  // outside the four-row viewport; object-list targets cannot steal that step.
  if(ctx==18&&s.focus.context==ctx&&s.focus.id>=EditorPropertyLayout067::FocusBase&&s.focus.id<EditorPropertyLayout067::ColorFocus&&!(pressed&(UiModality067::Left|UiModality067::Right))){
   if(const auto*def=p.syncPropertyLayout()){
    const int index=s.focus.id-EditorPropertyLayout067::FocusBase;
    const int next=index+int(bool(pressed&UiModality067::Down))-int(bool(pressed&UiModality067::Up));
    if(next!=index&&next>=0&&next<int(def->variable_count)){s.focus.id=EditorPropertyLayout067::FocusBase+next;ensure(p,s.focus.id);pressed&=~(UiModality067::Up|UiModality067::Down);}
   }
  }
  if(ctx==5&&s.focus.context==ctx&&s.focus.id>=3000000&&s.focus.id<4000000&&p.colorUi){
   const int index=s.focus.id-3000000,delta=int(bool(pressed&UiModality067::Right))-int(bool(pressed&UiModality067::Left))+int(EditorColorPickerArt::Columns)*(int(bool(pressed&UiModality067::Down))-int(bool(pressed&UiModality067::Up)));
   if(delta&&index+delta>=0&&index+delta<int(p.colorUi->visual.size())){s.focus.id=3000000+index+delta;ensure(p,s.focus.id);pressed&=~UiModality067::Directions;}
  }
  gather(p,s);int preferred=-1;
  if(ctx==7)preferred=40000+int(p.popupSelected);else if(ctx==4&&!p.fileEntries.empty())preferred=2000000+int(p.fileSelected);
  s.select(ctx,pressed,preferred);ensure(p,s.focus.id);gather(p,s);s.select(ctx,0);
  if(ctx==4){s.fileFocusGeneration=p.fileFocusGeneration;s.fileSelected=p.fileSelected;p.fileFocusPrepared=true;}
 }
};
}
namespace EditorBrowser {
struct FocusAccess067 {
 static void native(Browser&b){b.cursor={-100,-100};b.keyFocus=true;}
 static void prepare(Browser&b,EditorFocus067::State&s,unsigned pressed){using EditorFocus067::add;s.targets.clear();const auto mode=b.mode;
  if(mode==Browser::Mode::Error){add(s.targets,90000,288,344,192,40);add(s.targets,90001,520,344,192,40);}
  else if(mode==Browser::Mode::LevelResults){add(s.targets,90010,264,400,144,40);add(s.targets,90011,424,400,144,40);add(s.targets,90012,584,400,144,40);add(s.targets,90013,264,8,120,64);}
  else if(mode==Browser::Mode::Results){const float y=resultPose(b.animation,2)-24;if(y>=0&&y+64<=512)add(s.targets,90020,264,y,120,64);}
  s.select(100+unsigned(mode),pressed);
 }
};
}
