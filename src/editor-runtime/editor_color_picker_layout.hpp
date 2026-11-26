#pragma once
#include "editor_color_picker_name.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>
namespace EditorColorPickerArt {
struct Rect {
  float x = 0, y = 0, w = 0, h = 0;
  bool contains(double px, double py) const {
    return w > 0 && h > 0 && px >= x && py >= y && px < x + w && py < y + h;
  }
};
inline constexpr unsigned Columns = 9, RecentLimit = 9;
struct Layout {
  float w = 176, h = 293, unclampedWidth = 176;
  bool widthClipped = false;
  unsigned presetCount = 0, recentCount = 0;
  bool swatchesOpen = false, recentOpen = false, sourceButton = true;
  Rect sv, hue, sampler, sample, shape, mode[3], modeMenu, channelLabel[4],
      slider[4], spin[4], spinText[4], hex;
  Rect swatchesHeader, paletteMenu, presetGrid, add, recentHeader, recentGrid,
      paletteName;
  unsigned presetBegin068()const{return presetFirstRow068?presetFirstRow068*Columns-1:0;}
  unsigned presetEnd068()const{return std::min(presetCount,(presetFirstRow068+presetRows068)*Columns-1);}
  Rect presetPrevious068,presetNext068,presetRange068;
  float coreWidth068=0,presetColumn068=0;
  unsigned presetFirstRow068=0,presetRows068=0;
  Rect presetUnclipped068(unsigned index) const {
    if(index>=presetCount||!swatchesOpen)return {};
    const unsigned cell=index+1,row=cell/Columns,col=cell%Columns;
    if(presetRows068)return {presetColumn068+4.f+col*20.f,presetGrid.y+(int(row)-int(presetFirstRow068))*20.f,16,16};
    return preset(index);
  }
  Rect preset(unsigned index) const {
    if(presetRows068){const unsigned row=(index+1)/Columns;if(row<presetFirstRow068||row>=presetFirstRow068+presetRows068)return {};return presetUnclipped068(index);}
    if (index >= presetCount || !swatchesOpen)
      return {};
    const unsigned cell = index + 1, row = cell / Columns, col = cell % Columns;
    if (sourceButton)
      return {4.f + col * 20.f, presetGrid.y + row * 20.f, 16, 16};
    return {4.f + (col ? 29.f + (col - 1) * 20.f : 0),
            presetGrid.y + (row ? 29.f + (row - 1) * 20.f : 0),
            col ? 16.f : 25.f, row ? 16.f : 25.f};
  }
  Rect recent(unsigned index) const {
    if (index >= recentCount || !recentOpen)
      return {};
    return {presetColumn068+4.f + index * 20.f, recentGrid.y, 16, 16};
  }
};
inline Layout layout(unsigned presets, unsigned recent, bool swatchesOpen,
                     bool recentOpen, bool sourceButton = true,
                     float paletteLabelWidth = 0, float maxSourceWidth = 760) {
  Layout out;
  out.sourceButton = sourceButton;
  out.presetCount = presets;
  out.recentCount = std::min(recent, RecentLimit);
  out.swatchesOpen = swatchesOpen;
  out.recentOpen = recentOpen;
  out.w = std::max(
      {176.f,
       swatchesOpen && presets >= 8 ? (sourceButton ? 184.f : 193.f) : 0.f,
       recentOpen && out.recentCount == 9 ? 184.f : 0.f});
  const bool named = swatchesOpen && paletteLabelWidth > 0;
  out.unclampedWidth =
      std::max(out.w, named ? 8.f + std::ceil(paletteLabelWidth) : 0.f);
  out.w = std::min(out.unclampedWidth, std::max(176.f, maxSourceWidth));
  out.widthClipped = out.w < out.unclampedWidth;
  const float gridHeight =
                  (sourceButton ? 16.f : 25.f) + 20.f * (presets / Columns),
              gridY = 273.f + (named ? 27 : 0),
              recentY = 273.f + (named ? 27 : 0) +
                        (swatchesOpen ? gridHeight + 4 : 0);
  out.paletteName = named ? Rect{4, 273, out.w - 8, 23} : Rect{};
  out.h = recentY + 20 + (recentOpen ? (out.recentCount ? 16.f : 0.f) + 4 : 0);
  out.sv = {4, 4, out.w - 27, 64};
  out.hue = {out.w - 19, 4, 15, 64};
  out.sampler = {4, 72, 28, 16};
  out.sample = {36, 72, out.w - 72, 16};
  out.shape = {out.w - 32, 72, 28, 16};
  float sizes[] = {44, 41, 43}, available = out.w - 48;
  bool active[] = {true, true, true};
  unsigned count = 3;
  for (bool repeat = true; repeat && count;) {
    repeat = false;
    float unit = available / count;
    for (unsigned i = 0; i < 3; ++i)
      if (active[i] && sizes[i] > unit) {
        active[i] = false;
        available -= sizes[i];
        --count;
        repeat = true;
      }
  }
  if (count)
    for (unsigned i = 0; i < 3; ++i)
      if (active[i])
        sizes[i] = available / count;
  double cursor = 4;
  for (unsigned i = 0; i < 3; ++i) {
    out.mode[i] = {float(std::floor(cursor)), 92,
                   float(std::floor(cursor + sizes[i]) - std::floor(cursor)),
                   22};
    cursor += sizes[i] + 4;
  }
  out.modeMenu = {float(std::floor(cursor)), 92, 28, 22};
  for (unsigned i = 0; i < 4; ++i) {
    out.channelLabel[i] = {4, 118.f + 27 * i, 12, 23};
    out.slider[i] = {20, 121.f + 27 * i, out.w - 86, 16};
    out.spin[i] = {out.w - 62, 118.f + 27 * i, 58, 23};
    out.spinText[i] = {out.w - 62, 118.f + 27 * i, 40, 23};
  }
  out.hex = {51, 226, out.w - 55, 23};
  out.swatchesHeader = {4, 253, out.w - 28, 16};
  out.paletteMenu = {out.w - 20, 253, 16, 16};
  out.presetGrid = {4, gridY, out.w - 8, gridHeight};
  out.add = {4, gridY, sourceButton ? 16.f : 25.f, sourceButton ? 16.f : 25.f};
  out.recentHeader = {4, recentY, out.w - 8, 16};
  out.recentGrid = {4, recentY + 20, out.w - 8, out.recentCount ? 16.f : 0.f};
  return out;
}
// Original source geometry is retained in two readable columns.
inline constexpr float PresentationScale068=1.95f,ValueScale068=1.1f;
inline constexpr unsigned PresetRows068=7;
inline unsigned lastPresetRow068(unsigned presets){const unsigned rows=presets/Columns+1;return rows>PresetRows068?rows-PresetRows068:0;}
inline unsigned ensurePreset068(unsigned index,unsigned first){const unsigned row=(index+1)/Columns;if(row<first)return row;if(row>=first+PresetRows068)return row-PresetRows068+1;return first;}
inline Layout readableLayout068(unsigned presets,unsigned recent,bool savedOpen,bool recentOpen,bool named,unsigned first){
 auto out=layout(8,9,true,true,true,0,184);
 out.presetCount=presets;out.recentCount=std::min(recent,RecentLimit);out.swatchesOpen=savedOpen;out.recentOpen=recentOpen;
 out.coreWidth068=184;out.presetColumn068=192;out.w=376;out.h=249;
 out.presetFirstRow068=std::min(first,lastPresetRow068(presets));out.presetRows068=PresetRows068;
 out.swatchesHeader={196,4,156,18};out.paletteMenu={356,4,16,16};
 out.paletteName=named&&savedOpen?Rect{196,24,176,23}:Rect{};
 out.presetGrid={196,named?51.f:24.f,176,140};out.add=savedOpen&&!out.presetFirstRow068?Rect{196,out.presetGrid.y,16,16}:Rect{};
 out.presetPrevious068=out.presetFirstRow068?Rect{336,191,16,16}:Rect{};out.presetNext068=out.presetFirstRow068<lastPresetRow068(presets)?Rect{356,191,16,16}:Rect{};out.presetRange068={196,191,132,16};
 out.recentHeader={196,209,176,18};out.recentGrid={196,229,176,out.recentCount?16.f:0.f};
 return out;
}
inline Rect logicalRect068(Rect r,float x,float y){return {x+r.x*PresentationScale068,y+r.y*PresentationScale068,r.w*PresentationScale068,r.h*PresentationScale068};}
inline float source068(double coordinate,float origin){return float((coordinate-origin)/PresentationScale068);}
inline bool transformed068=false;inline float originX068=0,originY068=0;
inline float drawX068(float x){return transformed068?originX068+x*PresentationScale068:x;}
inline float drawY068(float y){return transformed068?originY068+y*PresentationScale068:y;}
inline float drawScale068(){return transformed068?PresentationScale068:1.f;}
struct DrawScope068{bool previous=transformed068;float x=originX068,y=originY068;
 DrawScope068(float px,float py){transformed068=true;originX068=px;originY068=py;}
 ~DrawScope068(){transformed068=previous;originX068=x;originY068=y;}
};
struct ShapeLayout {
  Rect container, plane, bar, square;
  bool barVisible = true;
};
inline constexpr float WheelRadius = .42f, WheelSquareHalf = .5939696961966999f;
inline ShapeLayout shapeLayout(const Layout &m, unsigned shape) {
  if (!shape)
    return {m.sv, m.sv, m.hue, m.sv, true};
  const Rect area = shape == 1 ? Rect{4, 4, (m.coreWidth068?m.coreWidth068:m.w) - 8, 64} : m.sv;
  const Rect plane{area.x + (area.w - 64) * .5f, 4, 64, 56};
  const float halfX = plane.w * .5f * WheelSquareHalf,
              halfY = plane.h * .5f * WheelSquareHalf;
  return {area,
          plane,
          m.hue,
          {plane.x + plane.w * .5f - halfX, plane.y + plane.h * .5f - halfY,
           halfX * 2, halfY * 2},
          shape != 1};
}
// Definitions are in editor_color_picker_art.hpp, included after
// editor_render.hpp. Coordinates are relative to the inner ColorPicker origin;
// popup margins are4.
inline bool drawBase(const Layout &, float sourceX, float sourceY,
                     unsigned mode = 0, bool hexVisible = true,
                     bool swatchesHover = false, bool recentHover = false,
                     unsigned addState = 0, const char *paletteName = "");
inline float paletteNameWidth(const char *text) {
  return NameFont::width(text);
}
inline Layout namedLayout(unsigned presets, unsigned recent, bool savedOpen,
                          bool recentOpen, const char *name,
                          bool sourceButton = true,
                          float maxSourceWidth = 760) {
  return layout(presets, recent, savedOpen, recentOpen, sourceButton,
                name && *name ? paletteNameWidth(name) : 0, maxSourceWidth);
}
inline bool drawPaletteName(const Layout &, float sourceX, float sourceY,
                            const char *name);
inline bool drawPlane(const Layout &, float sourceX, float sourceY,
                      unsigned shape, double hueDegrees, double saturation,
                      double value, double okLightness);
inline bool drawShape(const Layout &, float sourceX, float sourceY,
                      unsigned shape, bool hover = false);
inline bool drawKnob(const Rect &slider, float sourceX, float sourceY,
                     double fraction);
inline bool drawSwatch(const Rect &, float sourceX, float sourceY, double red,
                       double green, double blue, double alpha,
                       bool selected = false);
inline bool drawChecker(const Rect &, float sourceX, float sourceY);
inline bool drawRevert(const Rect &sample, float sourceX, float sourceY,
                       uint32_t tint = 0xffffffffu);
inline void drawText(const Rect &field, float sourceX, float sourceY,
                     const char *value, bool right = true);
inline constexpr uint32_t TextColor = 0xffdfdfdfu;
inline constexpr float TextAscent = 11, TextDescent = 3, TextMargin = 4,
                       PopupMargin = 4;
template <class Color> inline bool same(const Color &a, const Color &b) {
  return float(a.r) == float(b.r) && float(a.g) == float(b.g) &&
         float(a.b) == float(b.b) && float(a.a) == float(b.a);
}
// Saved presets are in source insertion/visual order; recent colors are newest
// first.
template <class Color>
inline void addPreset(std::vector<Color> &values, const Color &color) {
  auto at = std::find_if(values.begin(), values.end(),
                         [&](const Color &v) { return same(v, color); });
  if (at != values.end())
    values.erase(at);
  values.push_back(color);
}
template <class Color>
inline void addRecent(std::vector<Color> &values, const Color &color) {
  auto at = std::find_if(values.begin(), values.end(),
                         [&](const Color &v) { return same(v, color); });
  if (at != values.end())
    values.erase(at);
  values.insert(values.begin(), color);
  if (values.size() > RecentLimit)
    values.resize(RecentLimit);
}
template <class Color>
inline void erasePreset(std::vector<Color> &values, const Color &color) {
  auto at = std::find_if(values.begin(), values.end(),
                         [&](const Color &v) { return same(v, color); });
  if (at != values.end())
    values.erase(at);
}
template <class Color>
inline bool movePreset(std::vector<Color> &values, unsigned from, unsigned to) {
  if (from >= values.size() || to >= values.size())
    return false;
  Color value = values[from];
  values.erase(values.begin() + from);
  values.insert(values.begin() + to, value);
  return true;
}
} // namespace EditorColorPickerArt
