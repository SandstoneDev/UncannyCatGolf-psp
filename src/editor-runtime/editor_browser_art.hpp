#pragma once
#include "editor_browser_data.hpp"
namespace EditorBrowserArt {
enum Skin {
  CardNormal,
  CardHover,
  CardPressed,
  Description,
  SubNormal,
  SubHover,
  SubPressed,
  GoNormal,
  GoHover,
  GoPressed,
  RankPeak,
  RankSwag,
  RankOk,
  RankErm,
  RankAtrocious,
  RankNone
};
struct ClipGuard {
  EditorRender::Clip render = EditorRender::clip;
  std::array<int, 4> ui = EditorArt::uiScissor;
  ClipGuard() {
    EditorRender::clip = {float(std::max(36, ui[0])), float(std::max(0, ui[1])),
                          float(std::min(444, ui[2])),
                          float(std::min(272, ui[3]))};
  }
  ~ClipGuard() {
    EditorRender::clip = render;
    EditorArt::scissor(ui[0], ui[1], ui[2], ui[3]);
  }
};
inline bool drawNativeNine(unsigned frame, float x, float y, float w, float h,
                           uint32_t tint, float left = 3, float top = 5,
                           float right = 3, float bottom = 5) {
  const auto &f = Packed::Frames[frame];
  std::string file = ::companion(executable.c_str(), f.path);
  EditorImages::Info info{};
  if (!EditorRender::inspectImage(file.c_str(), info)) {
    EditorRender::fail("Cannot read original Browser nine-patch: " + file);
    return false;
  }
  float width = float(info.width) + (w - f.w) * EditorRender::Scale,
        height = float(info.height) + (h - f.h) * EditorRender::Scale;
  float dx[] = {0, left, width - right, width},
        dy[] = {0, top, height - bottom, height};
  float sx[] = {0, left, float(info.width) - right, float(info.width)},
        sy[] = {0, top, float(info.height) - bottom, float(info.height)};
  ClipGuard guard;
  for (unsigned row = 0; row < 3; ++row)
    for (unsigned col = 0; col < 3; ++col) {
      if (dx[col] >= dx[col + 1] || dy[row] >= dy[row + 1] ||
          sx[col] >= sx[col + 1] || sy[row] >= sy[row + 1])
        continue;
      EditorRender::customImage(
          file, info,
          {sx[col], sy[row], sx[col + 1] - sx[col], sy[row + 1] - sy[row]},
          [&](EditorRender::Vec2 p) {
            return EditorRender::uiVertex(
                {x + f.x +
                     (dx[col] + (p.x - sx[col]) * (dx[col + 1] - dx[col]) /
                                    (sx[col + 1] - sx[col])) /
                         EditorRender::Scale,
                 y + f.y +
                     (dy[row] + (p.y - sy[row]) * (dy[row + 1] - dy[row]) /
                                    (sy[row + 1] - sy[row])) /
                         EditorRender::Scale},
                0, 0);
          },
          tint);
    }
  return true;
}
inline bool draw(Skin skin, float x, float y, float w = 0, float h = 0,
                 uint32_t tint = 0xffffffffu) {
  if (unsigned(skin) >= std::size(Packed::Frames))
    return false;
  const auto &f = Packed::Frames[unsigned(skin)];
  if (skin == SubNormal || skin == SubHover || skin == SubPressed)
    return drawNativeNine(unsigned(skin), x, y, w > 0 ? w : f.w,
                          h > 0 ? h : f.h, tint);
  std::string file = ::companion(executable.c_str(), f.path);
  EditorImages::Info info{};
  if (!EditorRender::inspectImage(file.c_str(), info)) {
    EditorRender::fail("Cannot read original Browser skin: " + file);
    return false;
  }
  float sx = w > 0 ? w / f.w : 1, sy = h > 0 ? h / f.h : 1,
        scale = f.raw ? 1 : EditorRender::Scale;
  ClipGuard guard;
  EditorRender::customImage(
      file, info, {0, 0, float(info.width), float(info.height)},
      [&](EditorRender::Vec2 p) {
        return EditorRender::uiVertex(
            {x + (f.x + p.x / scale) * sx, y + (f.y + p.y / scale) * sy}, 0, 0);
      },
      tint);
  return true;
}
inline bool drawFocus(float x, float y, float w, float h,
                      uint32_t tint = 0xffffffffu) {
  return Packed::FocusFrame >= 0 &&
         drawNativeNine(unsigned(Packed::FocusFrame), x, y, w, h, tint);
}
inline bool drawScroll(bool horizontal, unsigned state, float x, float y,
                       float w, float h, uint32_t tint = 0xffffffffu) {
  if (state >= 5)
    return false;
  const auto &style = Packed::Scroll[horizontal ? 0 : 1][state];
  if (style.frame < 0)
    return true;
  const auto &f = Packed::Frames[unsigned(style.frame)];
  std::string file = ::companion(executable.c_str(), f.path);
  EditorImages::Info info{};
  if (!EditorRender::inspectImage(file.c_str(), info)) {
    EditorRender::fail("Cannot read original Browser scrollbar: " + file);
    return false;
  }
  x -= style.expandLeft;
  y -= style.expandTop;
  w += style.expandLeft + style.expandRight;
  h += style.expandTop + style.expandBottom;
  float sumX = style.left + style.right, sumY = style.top + style.bottom,
        fitX = sumX > w ? w / sumX : 1, fitY = sumY > h ? h / sumY : 1;
  float dx[] = {0, style.left * fitX, w - style.right * fitX, w},
        dy[] = {0, style.top * fitY, h - style.bottom * fitY, h};
  float sx[] = {0, style.left, float(info.width) - style.right,
                float(info.width)},
        sy[] = {0, style.top, float(info.height) - style.bottom,
                float(info.height)};
  ClipGuard guard;
  for (unsigned row = 0; row < 3; ++row)
    for (unsigned col = 0; col < 3; ++col) {
      if (dx[col] >= dx[col + 1] || dy[row] >= dy[row + 1] ||
          sx[col] >= sx[col + 1] || sy[row] >= sy[row + 1])
        continue;
      EditorRender::customImage(
          file, info,
          {sx[col], sy[row], sx[col + 1] - sx[col], sy[row + 1] - sy[row]},
          [&](EditorRender::Vec2 p) {
            return EditorRender::uiVertex(
                {x + dx[col] +
                     (p.x - sx[col]) * (dx[col + 1] - dx[col]) /
                         (sx[col + 1] - sx[col]),
                 y + dy[row] +
                     (p.y - sy[row]) * (dy[row + 1] - dy[row]) /
                         (sy[row + 1] - sy[row])},
                0, 0);
          },
          tint);
    }
  return true;
}
} // namespace EditorBrowserArt
