#pragma once
#include "editor_color_picker_layout.hpp"
#include "editor_color_state.hpp"
#include "editor_menu_layout.hpp"
#ifdef __PSP__
#include <pspkernel.h>
#else
#include <chrono>
#endif
#include "editor_palette_files.hpp"
namespace EditorPanelColor {
inline uint32_t micros() {
#ifdef __PSP__
  return sceKernelGetSystemTimeLow();
#else
  return uint32_t(std::chrono::duration_cast<std::chrono::microseconds>(
                      std::chrono::steady_clock::now().time_since_epoch())
                      .count());
#endif
}
using Color = ucg::editor::Color;
using Rect = EditorColorPickerArt::Rect;
inline bool same(Color a, Color b) { return EditorColorPickerArt::same(a, b); }
inline bool ensure(std::vector<Color> &v, unsigned n, std::string &e) {
  return n <= v.capacity() || EditorPaletteFiles::detail::reserve(v, n, e);
}
inline bool copy(const std::vector<Color> &from, std::vector<Color> &to,
                 std::string &e) {
  std::vector<Color> temp;
  if (!ensure(temp, unsigned(from.size()), e))
    return false;
  temp.insert(temp.end(), from.begin(), from.end());
  to.swap(temp);
  return true;
}
inline Color hsvColor(EditorColors::Hsv c) {
  float h = float(c.h / 360), s = float(c.s), v = float(c.v), a = float(c.a),
        at = (h - std::floor(h)) * 6, f = at - std::floor(at), p = v * (1 - s),
        q = v * (1 - s * f), t = v * (1 - s * (1 - f));
  switch (unsigned(at) % 6) {
  case 0:
    return {v, t, p, a};
  case 1:
    return {q, v, p, a};
  case 2:
    return {p, v, t, a};
  case 3:
    return {p, q, v, a};
  case 4:
    return {t, p, v, a};
  default:
    return {v, p, q, a};
  }
}
struct Context {
  EditorColors::State picker;
  unsigned shape = 0,presetScroll068=0;
  bool colorized = true, savedOpen = false, recentOpen = false, dirty = false;
  std::vector<Color> logical, visual, recent;
  std::string path, name;
  EditorColors::Hsv hsv{0, 0, 1, 1};
  ok_color::HSL okPlane{0, 0, 1};
  int dragKind = -1, focusChannel = -1, selectedPreset = -1,
      selectedRecent = -1;
  void setColor(Color c) {
    picker.set(c);
    hsv = EditorColors::toHsv(c);
    okPlane = picker.ok();
  }
  void mode(EditorColors::State::Mode m) { picker.mode = m; }
  unsigned actualShape() const {
    return picker.mode == EditorColors::State::Mode::OKHSL ? 3 : shape;
  }
  void chooseShape(unsigned s) {
    shape = s % 4;
    hsv = EditorColors::toHsv(picker.value);
    okPlane = picker.ok();
  }
  std::string label() const {
    return name + (dirty && !name.empty() ? "*" : "");
  }
  EditorColorPickerArt::Layout layout() const {
    return EditorColorPickerArt::readableLayout068(unsigned(visual.size()),unsigned(recent.size()),savedOpen,recentOpen,!name.empty(),presetScroll068);
  }
  void changedPalette() {
    if (!name.empty())
      dirty = true;
  }
  bool add(Color c, std::string *error = nullptr) {
    std::string e;
    auto found = [&](const std::vector<Color> &v) {
      return std::find_if(v.begin(), v.end(),
                          [&](Color a) { return same(a, c); }) != v.end();
    };
    const unsigned nl = unsigned(logical.size()) + !found(logical),
                   nv = unsigned(visual.size()) + !found(visual);
    if (nl > EditorPaletteFiles::MaxColors ||
        nv > EditorPaletteFiles::MaxColors || !ensure(logical, nl, e) ||
        !ensure(visual, nv, e)) {
      if (error)
        *error = e.empty() ? "Palette exceeds4096 colors" : e;
      return false;
    }
    EditorColorPickerArt::addPreset(logical, c);
    EditorColorPickerArt::addPreset(visual, c);
    selectedPreset = int(visual.size()) - 1;
    changedPalette();
    return true;
  }
  void erase(Color c) {
    EditorColorPickerArt::erasePreset(logical, c);
    auto removed = std::find_if(visual.begin(), visual.end(),
                                [&](Color v) { return same(v, c); });
    if (removed != visual.end()) {
      const int index = int(removed - visual.begin());
      if (selectedPreset == index)
        selectedPreset = -1;
      else if (selectedPreset > index)
        --selectedPreset;
      visual.erase(removed);
    }
    if (logical.empty()) {
      path.clear();
      name.clear();
      dirty = false;
    } else
      changedPalette();
  }
  bool move(unsigned from, unsigned to) {
    if (!EditorColorPickerArt::movePreset(visual, from, to))
      return false;
    if (selectedPreset == int(from))
      selectedPreset = int(to);
    else if (from < to && selectedPreset > int(from) &&
             selectedPreset <= int(to))
      --selectedPreset;
    else if (from > to && selectedPreset >= int(to) &&
             selectedPreset < int(from))
      ++selectedPreset;
    return true;
  }
  void addRecent(Color c) {
    std::string e;
    if (!ensure(recent, EditorColorPickerArt::RecentLimit, e))
      return;
    auto at = std::find_if(recent.begin(), recent.end(),
                           [&](Color a) { return same(a, c); });
    if (at != recent.end())
      recent.erase(at);
    if (recent.size() >= EditorColorPickerArt::RecentLimit)
      recent.pop_back();
    recent.insert(recent.begin(), c);
    selectedRecent = 0;
    auto preset = std::find_if(visual.begin(), visual.end(),
                               [&](Color v) { return same(v, c); });
    selectedPreset = preset == visual.end() ? -1 : int(preset - visual.begin());
  }
  void selectRecent(unsigned index) {
    if (index >= recent.size())
      return;
    Color value = recent[index];
    recent.erase(recent.begin() + index);
    recent.insert(recent.begin(), value);
    selectedRecent = 0;
  }
  void clear() {
    logical.clear();
    visual.clear();
    selectedPreset = -1;
    path.clear();
    name.clear();
    dirty = false;
    savedOpen = false;
  }
  static std::string baseName(const std::string &p) {
    auto start = p.find_last_of("/\\"), dot = p.find_last_of('.');
    start = start == std::string::npos ? 0 : start + 1;
    if (dot == std::string::npos || dot < start)
      dot = p.size();
    return p.substr(start, dot - start);
  }
  bool load(const std::string &file, std::string &e) {
    std::vector<Color> values, shown;
    if (!EditorPaletteFiles::load(file, values, e) || !copy(values, shown, e))
      return false;
    logical.swap(values);
    visual.swap(shown);
    selectedPreset = int(visual.size()) - 1;
    path = file;
    name = baseName(file);
    dirty = false;
    savedOpen = true;
    return true;
  }
  bool save(const std::string &file, std::string &e,
            const std::vector<Color> *presetCache = nullptr) {
    // Source saves get_presets(), then _update_presets() rebuilds children from
    // the process-wide cache; drag order is never serialized.
    const auto &refresh = presetCache ? *presetCache : logical;
    std::vector<Color> canonical, shown;
    if (!copy(refresh, canonical, e) || !copy(refresh, shown, e) ||
        !EditorPaletteFiles::save(file, logical, e))
      return false;
    logical.swap(canonical);
    visual.swap(shown);
    selectedPreset = int(visual.size()) - 1;
    path = file;
    name = baseName(file);
    dirty = false;
    savedOpen = true;
    return true;
  }
  bool hexVisible() const {
    const auto &c = picker.value;
    return c.r >= 0 && c.r <= 1 && c.g >= 0 && c.g <= 1 && c.b >= 0 && c.b <= 1;
  }
  bool editHex(std::string text) {
    while (!text.empty() &&
           std::isspace(static_cast<unsigned char>(text.front())))
      text.erase(text.begin());
    while (!text.empty() &&
           std::isspace(static_cast<unsigned char>(text.back())))
      text.pop_back();
    if (!text.empty() && text[0] == '#')
      text.erase(text.begin());
    bool digits = !text.empty() &&
                  std::all_of(text.begin(), text.end(),
                              [](unsigned char c) { return std::isxdigit(c); });
    if (digits) {
      if (text.size() == 1)
        text = std::string(6, text[0]);
      else if (text.size() == 2)
        text = text + text + text;
      else if (text.size() == 5)
        text.resize(4);
      else if (text.size() == 7)
        text.resize(6);
    }
    auto edit = picker;
    if (!edit.hex(text))
      return false;
    if (EditorColors::rgba(edit.value) == EditorColors::rgba(picker.value))
      return true;
    setColor(edit.value);
    return true;
  }
  bool editChannel(unsigned i, double n) {
    if (i >= 4 || picker.quantized(i, n) == picker.channel(i))
      return false;
    if (!picker.edit(i, n))
      return false;
    if (picker.mode == EditorColors::State::Mode::HSV) {
      hsv = {picker.channel(0), picker.channel(1) / 100,
             picker.channel(2) / 100, picker.value.a};
      okPlane = picker.ok();
    } else if (picker.mode == EditorColors::State::Mode::OKHSL) {
      okPlane = {float(picker.channel(0) / 360), float(picker.channel(1) / 100),
                 float(picker.channel(2) / 100)};
      hsv = EditorColors::toHsv(picker.value);
    } else {
      hsv = EditorColors::toHsv(picker.value);
      okPlane = picker.ok();
    }
    return true;
  }
  void commitPlane() {
    const float alpha = float(picker.value.a);
    Color c;
    if (actualShape() == 3) {
      auto rgb = ok_color::okhsl_to_srgb(okPlane);
      c = {rgb.r, rgb.g, rgb.b, alpha};
    } else {
      hsv.a = alpha;
      c = hsvColor(hsv);
    }
    picker.set(c);
  }
  bool beginPlane(const EditorColorPickerArt::Layout &m, double x, double y) {
    const auto g = EditorColorPickerArt::shapeLayout(m, actualShape());
    if (!g.plane.contains(x, y))
      return false;
    if (actualShape() == 0)
      dragKind = 0;
    else {
      const double dx = x - g.plane.x - g.plane.w / 2,
                   dy = y - g.plane.y - g.plane.h / 2, r = std::hypot(dx, dy),
                   radius = g.plane.w / 2;
      if (actualShape() == 1) {
        if (g.square.contains(x, y))
          dragKind = 0;
        else if (r >= radius * .84 && r <= radius)
          dragKind = 1;
        else
          return false;
      } else {
        if (r > radius)
          return false;
        dragKind = 2;
      }
    }
    return drag(m, x, y);
  }
  bool beginBar(const EditorColorPickerArt::Layout &m, double x, double y) {
    auto g = EditorColorPickerArt::shapeLayout(m, actualShape());
    if (!g.barVisible || !g.bar.contains(x, y))
      return false;
    dragKind = 3;
    return drag(m, x, y);
  }
  bool beginChannel(const EditorColorPickerArt::Layout &m, unsigned i, double x,
                    double y) {
    if (i >= 4 || !m.slider[i].contains(x, y))
      return false;
    dragKind = int(4 + i);
    drag(m, x, y);
    return true;
  }
  bool drag(const EditorColorPickerArt::Layout &m, double x, double y) {
    if (dragKind < 0)
      return false;
    if (dragKind >= 4) {
      unsigned i = unsigned(dragKind - 4);
      return editChannel(
          i, std::clamp((x - m.slider[i].x) / m.slider[i].w, 0., 1.) *
                 picker.maximum(i));
    }
    auto g = EditorColorPickerArt::shapeLayout(m, actualShape());
    if (dragKind == 0) {
      auto r = actualShape() == 1 ? g.square : g.plane;
      hsv.s = std::clamp((x - r.x) / r.w, 0., 1.);
      hsv.v = 1 - std::clamp((y - r.y) / r.h, 0., 1.);
    } else if (dragKind == 1 || dragKind == 2) {
      double dx = x - g.plane.x - g.plane.w / 2,
             dy = y - g.plane.y - g.plane.h / 2,
             h = std::atan2(dy, dx) / 6.283185307179586;
      if (h < 0)
        h += 1;
      hsv.h = float(h) * 360.;
      if (dragKind == 2) {
        hsv.s = std::clamp(std::hypot(dx, dy) / (g.plane.w / 2), 0., 1.);
        okPlane.h = float(h);
        okPlane.s = float(hsv.s);
      }
    } else {
      double f = std::clamp((y - g.bar.y) / g.bar.h, 0., 1.);
      if (actualShape() == 0)
        hsv.h = float(f) * 360.;
      else {
        hsv.v = 1 - f;
        okPlane.l = float(1 - f);
      }
    }
    commitPlane();
    return true;
  }
  void endDrag() {
    if (dragKind >= 0)
      addRecent(picker.value);
    dragKind = -1;
  }
  std::vector<EditorMenuArt::Item> modeItems() const {
    std::vector<EditorMenuArt::Item> out;
    for (unsigned i = 0; i < 4; ++i) {
      EditorMenuArt::Item v;
      v.text = std::array<const char *, 4>{"RGB", "HSV", "RAW", "OKHSL"}[i];
      v.id = int(i);
      v.radio = true;
      v.checked = unsigned(picker.mode) == i;
      out.push_back(v);
    }
    EditorMenuArt::Item sep;
    sep.separator = true;
    sep.id = 4;
    out.push_back(sep);
    EditorMenuArt::Item flag;
    flag.text = "Colorized Sliders";
    flag.id = 5;
    flag.checkable = true;
    flag.checked = colorized;
    out.push_back(flag);
    return out;
  }
  std::vector<EditorMenuArt::Item> shapeItems() const {
    std::vector<EditorMenuArt::Item> out;
    for (unsigned i = 0; i < 4; ++i) {
      EditorMenuArt::Item v;
      v.text = std::array<const char *, 4>{"HSV Rectangle", "HSV Wheel",
                                           "VHS Circle", "OKHSL Circle"}[i];
      v.id = int(i);
      v.radio = true;
      v.checked = shape == i;
      v.icon = static_cast<EditorMenuArt::Icon>(
          unsigned(EditorMenuArt::Icon::HsvRectangle) + i);
      out.push_back(v);
    }
    return out;
  }
  std::vector<EditorMenuArt::Item> paletteItems() const {
    std::vector<EditorMenuArt::Item> out;
    auto add = [&](const char *text, int id, EditorMenuArt::Icon icon) {
      EditorMenuArt::Item v;
      v.text = text;
      v.id = id;
      v.icon = icon;
      out.push_back(v);
    };
    if (!logical.empty())
      add("Save", 0, EditorMenuArt::Icon::Save);
    if (!path.empty())
      add("Save As", 1, EditorMenuArt::Icon::Save);
    add("Load", 2, EditorMenuArt::Icon::Load);
    if (!logical.empty())
      add("Clear", 4, EditorMenuArt::Icon::Clear);
    return out;
  }
};
// Definitions live in editor_panel_color_art.hpp, after Renderer and PickerArt.
inline void drawChannels(const EditorColorPickerArt::Layout &, float, float,
                         const Context &, int hover = -1);
inline void drawSample(const EditorColorPickerArt::Rect &, float, float,
                       Color oldColor, Color color);
} // namespace EditorPanelColor
