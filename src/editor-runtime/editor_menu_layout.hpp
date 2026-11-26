#pragma once
#include "editor_color_picker_layout.hpp"
#include "editor_legacy_fonts_data.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <string>
#include <tuple>
#include <vector>
namespace EditorMenuArt {
using Rect = EditorColorPickerArt::Rect;
enum class Icon {
  None,
  Song,
  HsvRectangle,
  HsvWheel,
  HsvCircle,
  OkhslCircle,
  Save,
  Load,
  Clear
};
struct Item {
  std::string text;
  int id = -1;
  Icon icon = Icon::None;
  bool checkable = false, radio = false, checked = false, disabled = false,
       separator = false, submenu = false;
  unsigned indent = 0;
  std::string accelerator;
  uint32_t iconTint = 0xffffffffu;
};
struct IconInfo {
  const char *name;
  float w, h;
};
inline IconInfo iconInfo(Icon icon) {
  switch (icon) {
  case Icon::Song:
    return {"MenuItem_song_0", 10, 10};
  case Icon::HsvRectangle:
    return {"MenuItem_ColorShape_0", 16, 16};
  case Icon::HsvWheel:
    return {"MenuItem_ColorShape_1", 16, 16};
  case Icon::HsvCircle:
    return {"MenuItem_ColorShape_2", 16, 16};
  case Icon::OkhslCircle:
    return {"MenuItem_ColorShape_3", 16, 16};
  case Icon::Save:
    return {"MenuItem_ColorPalette_0", 16, 16};
  case Icon::Load:
    return {"MenuItem_ColorPalette_1", 16, 16};
  case Icon::Clear:
    return {"MenuItem_ColorPalette_2", 16, 16};
  default:
    return {nullptr, 0, 0};
  }
}
namespace Font {
using Glyph = EditorRender::LegacyFonts::Glyph;
using Kern = EditorRender::LegacyFonts::Kern;
inline const Glyph *glyph(unsigned code) {
  const auto &f = EditorRender::LegacyFonts::Fonts[3];
  for (unsigned i = 0; i < f.count; ++i) {
    const auto &g = EditorRender::LegacyFonts::Glyphs[f.first + i];
    if (g.code == code)
      return &g;
  }
  return nullptr;
}
inline const Kern *pair(unsigned a, unsigned b) {
  const auto &pairs = EditorRender::LegacyFonts::Kerning;
  size_t l = 0, r = std::size(pairs);
  while (l < r) {
    size_t m = (l + r) / 2;
    const auto &k = pairs[m];
    if (std::make_tuple(k.font, k.left, k.right) < std::make_tuple(3u, a, b))
      l = m + 1;
    else
      r = m;
  }
  if (l < std::size(pairs)) {
    const auto &k = pairs[l];
    if (k.font == 3 && k.left == a && k.right == b)
      return &k;
  }
  return nullptr;
}
struct Metrics {
  float width = 0, ascent = 12, descent = 3, height = 15;
};
inline Metrics metrics(const char *text) {
  Metrics result;
  std::vector<unsigned> codes;
  if (text)
    while (*text)
      codes.push_back(EditorColorPickerArt::NameFont::codepoint(text));
  float pen = 0, carry = 0;
  unsigned face = UINT32_MAX;
  for (unsigned i = 0; i < codes.size(); ++i) {
    unsigned cp = codes[i];
    const auto *g = glyph(cp);
    if (!g)
      g = glyph(63);
    if (!g)
      continue;
    if (g->face != face || cp == ' ' || cp == '\t')
      carry = 0;
    const Kern *before = nullptr, *after = nullptr;
    if (i) {
      const auto *p = glyph(codes[i - 1]);
      if (p && p->face == g->face)
        before = pair(codes[i - 1], cp);
    }
    if (i + 1 < codes.size()) {
      const auto *n = glyph(codes[i + 1]);
      if (n && n->face == g->face)
        after = pair(cp, codes[i + 1]);
    }
    float advance =
        g->advance + (before ? before->db : 0) + (after ? after->da : 0);
    if (g->snap) {
      float full = carry + advance;
      advance = std::round(full);
      carry = full - advance;
    } else
      carry = 0;
    pen += advance;
    face = g->face;
    result.ascent = std::max(result.ascent, g->ascent);
    result.descent = std::max(result.descent, g->descent);
  }
  result.width = std::ceil(pen)*2.25f;result.ascent*=2.25f;result.descent*=2.25f;
  result.height = result.ascent + result.descent;
  return result;
}
} // namespace Font
struct Row {
  Rect hit, hover, text, check, icon, submenu, separatorLeft, separatorRight,
      accelerator;
  float baseline = 0, contentHeight = 0;
  unsigned index = 0;
};
struct Layout {
  float w = 0, h = 0, fullHeight = 0, unclampedWidth = 0, scroll = 0,
        scrollMax = 0;
  bool widthClipped = false, heightClipped = false;
  Rect content;
  std::vector<Row> rows;
};
inline Layout layout(const std::vector<Item> &items, float minWidth = 0,
                     float maxWidth = 760, float maxHeight = 504,
                     float scroll = 0) {
  Layout out;
  float iconWidth = 0, checkWidth = 0, maxText = 0, maxAccelerator = 0;
  for (const auto &i : items) {
    const auto icon = iconInfo(i.icon);
    iconWidth = std::max(iconWidth, icon.w);
    if ((i.checkable || i.radio) && !i.separator)
      checkWidth = 16;
    maxText = std::max(maxText, Font::metrics(i.text.c_str()).width +
                                    i.indent * 10 + (i.submenu ? 8 : 0));
    if (!i.accelerator.empty())
      maxAccelerator = std::max(maxAccelerator,
                                8 + Font::metrics(i.accelerator.c_str()).width);
  }
  out.unclampedWidth =
      std::max(minWidth, std::ceil(12 + maxText + iconWidth + checkWidth +
                                   maxAccelerator));
  out.w = std::min(out.unclampedWidth, std::max(12.f, maxWidth));
  out.widthClipped = out.w < out.unclampedWidth;
  float height = 0;
  for (const auto &i : items) {
    const auto f = Font::metrics(i.text.c_str());
    const auto icon = iconInfo(i.icon);
    height +=
        4 + std::max({i.separator && i.text.empty() ? 0.f : f.height, icon.h,
                      (i.checkable || i.radio) && !i.separator ? 12.f : 0.f});
  }
  out.fullHeight = 8 + height;
  out.h = std::min(out.fullHeight, std::max(8.f, maxHeight));
  out.heightClipped = out.h < out.fullHeight;
  out.scrollMax = std::max(0.f, out.fullHeight - out.h);
  out.scroll = std::clamp(scroll, 0.f, out.scrollMax);
  out.content = {4, 4, out.w - 8, out.h - 8};
  float top = 4 - out.scroll;
  for (unsigned at = 0; at < items.size(); ++at) {
    const auto &i = items[at];
    const auto f = Font::metrics(i.text.c_str());
    const auto icon = iconInfo(i.icon);
    const float textHeight = i.separator && i.text.empty() ? 0 : f.height,
                h = std::max(
                    {textHeight, icon.h,
                     (i.checkable || i.radio) && !i.separator ? 12.f : 0.f}),
                contentY = top + 2, start = 6 + i.indent * 10;
    Row r;
    r.index = at;
    r.contentHeight = h;
    r.hit = r.hover = {4, top, out.w - 8, h + 4};
    r.text = {
        i.separator ? 4 + std::floor((out.content.w - f.width) * .5f)
                    : start + checkWidth + (iconWidth > 0 ? iconWidth + 4 : 0),
        contentY + std::floor((h - textHeight) * .5f), f.width, textHeight};
    r.baseline = r.text.y + f.ascent;
    r.check = {start, contentY + std::floor((h - 12) * .5f), 12, 12};
    r.icon = {start + checkWidth, contentY + std::floor((h - icon.h) * .5f),
              icon.w, icon.h};
    r.submenu = {out.w - 18, contentY + std::floor(h - 16) * .5f, 8, 16};
    r.accelerator = {out.w - 10 - Font::metrics(i.accelerator.c_str()).width,
                     r.text.y, Font::metrics(i.accelerator.c_str()).width,
                     textHeight};
    if (i.separator) {
      float center = std::floor(out.content.w * .5f),
            size = f.width + 8 + (icon.w > 0 ? icon.w + 4 : 0),
            left = center - std::floor(size * .5f),
            right = center + std::floor(size * .5f);
      if (i.text.empty() && icon.w == 0)
        r.separatorLeft = {4, contentY, out.content.w, 0};
      else {
        r.separatorLeft = {4, contentY + std::floor(h * .5f),
                           std::max(0.f, left), 0};
        r.separatorRight = {4 + right, contentY + std::floor(h * .5f),
                            std::max(0.f, out.content.w - right), 0};
      }
    }
    out.rows.push_back(r);
    top += h + 4;
  }
  return out;
}
inline int hit(const Layout &m, const std::vector<Item> &items, double x,
               double y) {
  if (!m.content.contains(x, y))
    return -1;
  for (const auto &r : m.rows)
    if (r.hit.contains(x, y) && r.index < items.size() &&
        !items[r.index].disabled && !items[r.index].separator)
      return int(r.index);
  return -1;
}
struct Point {
  float x, y;
};
inline Point anchor(Rect control, float originX = 0, float originY = 0) {
  return {originX + control.x, originY + control.y + control.h};
}
inline bool drawMenu(const Layout &, const std::vector<Item> &, float sourceX,
                     float sourceY, int hover = -1);
inline constexpr uint32_t NormalColor = 0xffdfdfdfu, HoverColor = 0xffdfdfdfu,
                          DisabledColor = 0xcc666666u,
                          AcceleratorColor = 0xccb3b3b3u;
} // namespace EditorMenuArt
