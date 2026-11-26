#pragma once
#include <algorithm>
#include <array>
#include <string>
namespace EditorSettingsArt {
enum Widget {
  Fullscreen,
  Sensitivity,
  Invert,
  Buffer,
  Master,
  Music,
  Sound,
  Save,
  Back,
  Count
};
struct Rect {
  float x = 0, y = 0, w = 0, h = 0;
  bool contains(double px, double py) const {
    return w > 0 && h > 0 && px >= x && py >= y && px < x + w && py < y + h;
  }
};
struct Values {
  bool fullscreen = true, invert = false, buffer = false;
  double sensitivity = 1, master = 1, music = 1, sound = 1;
};
#include "editor_settings_rects.hpp"
inline unsigned firstRow068=0;
inline std::array<Rect,Count> viewRects068{};
inline void setFirstRow068(unsigned first){
 firstRow068=std::min(1u,first);constexpr float s=17.f/32;
 for(unsigned i=0;i<7;++i)viewRects068[i]=i>=firstRow068&&i<firstRow068+6?Rect{263/s,(52+25*float(i-firstRow068))/s,130/s,23/s}:Rect{};
 viewRects068[Save]={204/s,240/s,136/s,25/s};viewRects068[Back]={140/s,8/s,64/s,23/s};
}
inline const Rect &rect(bool ru, Widget id) {
 if(ru)return Rects[1][unsigned(id)];
 setFirstRow068(firstRow068);return viewRects068[unsigned(id)];
}
inline double sliderFraction(bool ru, Widget id, double sourceX) {
  if(!ru)return std::clamp((sourceX*(17./32)-284)/87.,0.,1.);
  const auto &r = rect(ru, id);
  double inset = SliderInset[unsigned(ru)][unsigned(id)],
         width = r.w - 2 * inset;
  return width > 0 ? std::clamp((sourceX - r.x - inset) / width, 0., 1.) : 0.;
}
inline const std::string &getError();
inline void unload();
inline bool drawBack(const char *, bool, double = 0, float sourceY = 8);
inline bool draw(const char *, bool, const Values &, int = -1, int = -1,
                 bool = false, double = 0);
} // namespace EditorSettingsArt
#include "editor_settings_runtime.hpp"
