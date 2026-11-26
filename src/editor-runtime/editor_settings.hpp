#pragma once
#include "ucg_editor_model.hpp"
namespace EditorSettings {
struct Values {
  double sensitivity=1, master=1, music=1, sounds=1;
  bool invert=false, buffer=false;
  ucg::editor::Value json()const {
    auto root=ucg::editor::Value::dict();
    root["power_sensitivity"]=ucg::editor::Value::number(sensitivity);
    root["master_volume"]=ucg::editor::Value::number(master);
    root["music_volume"]=ucg::editor::Value::number(music);
    root["sound_volume"]=ucg::editor::Value::number(sounds);
    root["invert_shot"]=ucg::editor::Value(invert);
    root["buffer_shot"]=ucg::editor::Value(buffer);
    return root;
  }
  bool read(const ucg::editor::Value&root,std::string&error) {
    Values candidate;
    auto number=[&](const char*key,double&out,double max) {
      const auto*p=root.find(key);if(!p)return true;
      if(p->kind!=ucg::editor::Value::Kind::Number||!std::isfinite(p->numeric())||p->numeric()<0||p->numeric()>max){error=std::string("Invalid editor setting: ")+key;return false;}
      out=p->numeric();return true;
    };
    auto boolean=[&](const char*key,bool&out) {
      const auto*p=root.find(key);if(!p)return true;
      if(p->kind!=ucg::editor::Value::Kind::Bool){error=std::string("Invalid editor setting: ")+key;return false;}
      out=p->boolean;return true;
    };
    if(!number("power_sensitivity",candidate.sensitivity,2)||!number("master_volume",candidate.master,1)||!number("music_volume",candidate.music,1)||!number("sound_volume",candidate.sounds,1)||!boolean("invert_shot",candidate.invert)||!boolean("buffer_shot",candidate.buffer))return false;
    *this=candidate;error.clear();return true;
  }
};
} // namespace EditorSettings
