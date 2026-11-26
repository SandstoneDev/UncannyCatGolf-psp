#pragma once
#include "editor_settings.hpp"
#include "editor_settings_art.hpp"
namespace EditorSettings {
struct Panel {
  Values values;
  bool active=false,dirty=false,keyFocus=false;
  int focused=EditorSettingsArt::Sensitivity,hover=-1,pressedWidget=-1;
  std::string path,error;
  std::function<void()> onChange,onClose;
  void open(const std::string&file) {
    path=file;error.clear();
    if(EditorFiles::exists(path)) {
      ucg::editor::Document doc;
      if(ucg::editor::load_file(path,doc,error))values.read(doc.root,error);
    }
  }
  void show(){EditorSettingsArt::setFirstRow068(0);active=true;keyFocus=false;hover=pressedWidget=-1;focused=EditorSettingsArt::Sensitivity;}
  void close(){active=false;if(onClose)onClose();}
  void update(const EditorInput::Actions&a,ucg::editor::Vec2 at,ucg::editor::Vec2 before,unsigned pressed) {
    using namespace EditorSettingsArt;
    if(a.back){close();return;}
    if(at.x!=before.x||at.y!=before.y)keyFocus=false;
    constexpr int order[]={Sensitivity,Invert,Buffer,Master,Music,EditorSettingsArt::Sound,Save,Back};
    const bool up=pressed&EditorInput::Up,down=pressed&EditorInput::Down;
    if(up!=down){unsigned index=0;while(index+1<std::size(order)&&order[index]!=focused)++index;focused=order[(index+(down?1:std::size(order)-1))%std::size(order)];keyFocus=true;}
    if(keyFocus&&focused>=0&&focused<7){unsigned first=EditorSettingsArt::firstRow068;if(unsigned(focused)<first)first=unsigned(focused);if(unsigned(focused)>=first+6)first=unsigned(focused)-5;EditorSettingsArt::setFirstRow068(first);}
    const double nativeX=at.x*(17./32),nativeY=at.y*(17./32);
    if(a.primary&&nativeX>=402&&nativeX<412&&nativeY>=53&&nativeY<200){EditorSettingsArt::setFirstRow068(nativeY>=126?1:0);keyFocus=false;}
    hover=-1;
    for(unsigned i=0;i<Count;++i)if(rect(russian(),static_cast<Widget>(i)).contains(float(at.x),float(at.y)))hover=int(i);
    if(pressed&(EditorInput::Left|EditorInput::Right))keyFocus=true;
    if(keyFocus)hover=focused;
    pressedWidget=a.primary?hover:-1;
    const auto old=values.json();
    double*value=nullptr;double maximum=1,step=.05;
    if(hover==Sensitivity){value=&values.sensitivity;maximum=2;step=.2;}
    else if(hover==Master)value=&values.master;
    else if(hover==Music)value=&values.music;
    else if(hover==EditorSettingsArt::Sound)value=&values.sounds;
    if(value){
      const auto&r=rect(russian(),static_cast<Widget>(hover));
      if(a.primary&&!keyFocus&&r.w>0)*value=std::clamp(std::round(sliderFraction(russian(),static_cast<Widget>(hover),at.x)*maximum/step)*step,0.,maximum);
      const bool left=pressed&EditorInput::Left,right=pressed&EditorInput::Right;
      if(left!=right)*value=std::clamp(std::round((*value+(right?step:-step))/step)*step,0.,maximum);
    }
    if(a.primaryDown){
      if(hover==Invert)values.invert=!values.invert;
      else if(hover==Buffer)values.buffer=!values.buffer;
      else if(hover==Back){close();return;}
      else if(hover==Save&&dirty){ucg::editor::Document doc;doc.root=values.json();if(ucg::editor::save_atomic(path,doc,error)){dirty=false;EditorCues::play(332);}}
    }
    if(ucg::editor::write_json(old)!=ucg::editor::write_json(values.json())){
      dirty=true;error.clear();if(onChange)onChange();
      if(hover==Master||hover==EditorSettingsArt::Sound)EditorCues::raw(357,0,1);
    }
  }
  void draw(double seconds){
    EditorSettingsArt::Values v;
    v.invert=values.invert;v.buffer=values.buffer;v.sensitivity=values.sensitivity;
    v.master=values.master;v.music=values.music;v.sound=values.sounds;
    EditorSettingsMainUi068::diagnostic=error.empty()?nullptr:error.c_str();
    if(!EditorSettingsArt::draw(executable.c_str(),russian(),v,hover,pressedWidget,dirty,seconds)){error=EditorSettingsArt::getError();EditorSettingsMainUi068::diagnostic=error.c_str();EditorSettingsMainUi068::description(unsigned(EditorSettingsArt::Save),seconds);}
  }
};
} // namespace EditorSettings
