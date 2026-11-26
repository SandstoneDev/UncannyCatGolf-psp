#pragma once
#include "editor_color_picker_art.hpp"
#include "editor_menu_layout.hpp"
namespace EditorMenuArt {
inline bool skinImage(const char *name, const Rect &dest,
                      EditorArt::Rect region = {0, 0, 0, 0},
                      uint32_t tint = 0xffffffffu) {
  if (dest.w <= 0 || dest.h <= 0)
    return true;
  const auto path = ::companion(
      executable.c_str(),
      (std::string("data/world/editor-aux/menus/") + name + ".png").c_str());
  EditorImages::Info info{};
  if (!EditorRender::inspectImage(path.c_str(), info)) {
    EditorRender::fail("Cannot read original PopupMenu skin: " + path);
    return false;
  }
  if (region.w <= 0 || region.h <= 0)
    region = {0, 0, float(info.width), float(info.height)};
  const auto errors = EditorRender::failedImages;
  EditorColorPickerArt::ClipGuard guard;
  EditorRender::customImage(
      path, info, region,
      [&](EditorRender::Vec2 p) {
        return EditorRender::uiVertex(
            {dest.x + (p.x - region.x) * dest.w / region.w,
             dest.y + (p.y - region.y) * dest.h / region.h},
            0, 0);
      },
      tint);
  return errors == EditorRender::failedImages;
}
inline bool nine(const char *name, const Rect &dest, float sourceW,
                 float sourceH) {
  const float dx[] = {0, 4, dest.w - 4, dest.w},
              dy[] = {0, 4, dest.h - 4, dest.h},
              sx[] = {0, 4, sourceW - 4, sourceW},
              sy[] = {0, 4, sourceH - 4, sourceH};
  bool ok = true;
  for (unsigned row = 0; row < 3; ++row)
    for (unsigned col = 0; col < 3; ++col)
      ok = skinImage(name,
                     {dest.x + dx[col], dest.y + dy[row], dx[col + 1] - dx[col],
                      dy[row + 1] - dy[row]},
                     {sx[col], sy[row], sx[col + 1] - sx[col],
                      sy[row + 1] - sy[row]}) &&
           ok;
  return ok;
}
inline Rect translated(Rect r, float x, float y) {
  r.x += x;
  r.y += y;
  return r;
}
inline bool drawMenu(const Layout &m, const std::vector<Item> &items, float x,
                     float y, int hover) {
  bool ok = nine("MenuStyle_panel", {x, y, m.w, m.h}, 64, 64);
  const auto previous = EditorArt::uiScissor;
  const Rect content = translated(m.content, x, y);
  auto pixelBound = [](float v) {
    return int(std::ceil(v * EditorRender::Scale - .5f));
  };
  const int left = std::max(previous[0], 36 + pixelBound(content.x)),
            top = std::max(previous[1], pixelBound(content.y)),
            right =
                std::min(previous[2], 36 + pixelBound(content.x + content.w)),
            bottom = std::min(previous[3], pixelBound(content.y + content.h));
  if (left >= right || top >= bottom)
    return ok;
  EditorArt::scissor(left, top, right, bottom);
  for (const auto &r : m.rows) {
    if (r.index >= items.size() || r.hit.y + r.hit.h <= m.content.y ||
        r.hit.y >= m.content.y + m.content.h)
      continue;
    const auto &i = items[r.index];
    if (int(r.index) == hover)
      ok = nine("MenuStyle_hover", translated(r.hover, x, y), 64, 32) && ok;
    if (i.separator) {
      if (r.separatorLeft.w > 0)
        ok = skinImage("MenuStyle_separator",
                       {x + r.separatorLeft.x, y + r.separatorLeft.y - 4,
                        r.separatorLeft.w, 8}) &&
             ok;
      if (r.separatorRight.w > 0)
        ok = skinImage("MenuStyle_separator",
                       {x + r.separatorRight.x, y + r.separatorRight.y - 4,
                        r.separatorRight.w, 8}) &&
             ok;
    }
    if ((i.checkable || i.radio) && !i.separator) {
      const char *name =
          i.radio
              ? (i.checked ? (i.disabled ? "MenuIcon_radio_checked_disabled"
                                         : "MenuIcon_radio_checked")
                           : (i.disabled ? "MenuIcon_radio_unchecked_disabled"
                                         : "MenuIcon_radio_unchecked"))
              : (i.checked ? (i.disabled ? "MenuIcon_checked_disabled"
                                         : "MenuIcon_checked")
                           : (i.disabled ? "MenuIcon_unchecked_disabled"
                                         : "MenuIcon_unchecked"));
      ok = skinImage(name, translated(r.check, x, y), {0, 0, 0, 0},
                     i.iconTint) &&
           ok;
    }
    const auto icon = iconInfo(i.icon);
    if (icon.name)
      ok = skinImage(icon.name, translated(r.icon, x, y), {0, 0, 0, 0},
                     i.iconTint) &&
           ok;
    if (i.submenu)
      ok = skinImage("MenuIcon_submenu", translated(r.submenu, x, y),
                     {0, 0, 0, 0}, i.iconTint) &&
           ok;
    if (!i.text.empty())
      EditorArt::textScaled(3, i.text.c_str(), x + r.text.x, y + r.baseline,
                      i.disabled              ? DisabledColor
                      : int(r.index) == hover ? HoverColor
                                              : NormalColor,std::max(1.f,m.content.x+m.content.w-r.text.x-(i.accelerator.empty()?0:r.accelerator.w+8)-(i.submenu?16:0)),2.25f);
    if (!i.accelerator.empty())
      EditorArt::textScaled(3, i.accelerator.c_str(), x + r.accelerator.x,
                      y + r.baseline,
                      int(r.index) == hover ? HoverColor : AcceleratorColor,r.accelerator.w,2.25f);
  }
  EditorArt::scissor(previous[0], previous[1], previous[2], previous[3]);
  return ok;
}
} // namespace EditorMenuArt
