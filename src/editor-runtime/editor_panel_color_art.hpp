#pragma once
#include "editor_color_picker_art.hpp"
#include "editor_panel_color.hpp"
#include "editor_raster.hpp"
namespace EditorPanelColor {
namespace ChannelArt {
using RGB = EditorColorPickerArt::Raster::RGB;
inline RGB mix(RGB a, RGB b, float f) {
  return {a.r + (b.r - a.r) * f, a.g + (b.g - a.g) * f, a.b + (b.b - a.b) * f};
}
inline RGB rgb(Color c) { return {float(c.r), float(c.g), float(c.b)}; }
inline ok_color::Lab lab(RGB c) {
  return ok_color::linear_srgb_to_oklab(
      {ok_color::srgb_transfer_function_inv(c.r),
       ok_color::srgb_transfer_function_inv(c.g),
       ok_color::srgb_transfer_function_inv(c.b)});
}
inline RGB fromLab(ok_color::Lab c) {
  auto x = ok_color::oklab_to_linear_srgb(c);
  return {ok_color::srgb_transfer_function(x.r),
          ok_color::srgb_transfer_function(x.g),
          ok_color::srgb_transfer_function(x.b)};
}
inline uint32_t pixel(RGB c, float a = 1) {
  auto b = [](float v) {
    if (!std::isfinite(v))
      return 0u;
    return unsigned(std::lround(std::clamp(v, 0.f, 1.f) * 255.f));
  };
  return b(c.r) | (b(c.g) << 8) | (b(c.b) << 16) | (b(a) << 24);
}
inline RGB unpack(uint32_t c) {
  return {float(c & 255) / 255, float((c >> 8) & 255) / 255,
          float((c >> 16) & 255) / 255};
}
inline uint32_t checker[16 * 16]{};
inline bool checkerReady = false;
inline bool checkerLoad() {
  if (checkerReady)
    return true;
  if (!EditorImages::read(
          EditorColorPickerArt::file("PickerIcon_sample_bg").c_str(),
          {0, 0, 16, 16}, checker, 16)) {
    EditorRender::fail("Cannot read original ColorPicker checker");
    return false;
  }
  checkerReady = true;
  return true;
}
inline uint32_t check(unsigned x, unsigned y) {
  return checker[(y % 16) * 16 + x % 16];
}
inline RGB over(RGB color, float alpha, RGB background) {
  return mix(background, color, std::clamp(alpha, 0.f, 1.f));
}
// One shared strip, synchronized before reuse; no per-channel image cache.
alignas(64) inline uint32_t linear[256 * 16]{};
alignas(64) inline uint8_t packed[256 * 16 * 4]{};
template <class Sample>
inline void strip(Rect r, float x, float y, Sample sample) {
  if (r.w <= 0 || r.h <= 0)
    return;
  const unsigned width = unsigned(std::ceil(r.w)),
                 height = unsigned(std::ceil(r.h));
  if (width > 760 || height > 16) {
    EditorRender::fail("ColorPicker strip dimensions exceed bounds");
    return;
  }
  for (unsigned start = 0; start < width; start += 256) {
    const unsigned cw = std::min(256u, width - start);
    flushBatch();
    for (unsigned yy = 0; yy < height; ++yy)
      for (unsigned xx = 0; xx < cw; ++xx)
        linear[yy * 256 + xx] = sample(start + xx, yy);
    const auto d = EditorRaster::pack(linear, 256, packed, cw, height);
    sceKernelDcacheWritebackRange(packed, d.bytes);
    EditorRender::markTextureDirty();
    EditorColorPickerArt::ClipGuard guard;
    const float left = std::round(x + r.x) + start, top = std::round(y + r.y),
                w = std::min(float(cw), r.w - start), h = r.h;
    std::array<EditorRender::Vertex, 4> q{
        EditorColorPickerArt::vertex068({left, top}, 0, 0),
        EditorColorPickerArt::vertex068({left + w, top}, w, 0),
        EditorColorPickerArt::vertex068({left + w, top + h}, w, h),
        EditorColorPickerArt::vertex068({left, top + h}, 0, h)};
    EditorRender::polygonTexture(packed, GU_PSM_8888, d.width, d.height,
                                 q.data(), 4, 0xffffffffu);
    flushBatch();
  }
}
inline bool rawImage(const char *name, Rect dest,
                     EditorArt::Rect region = {0, 0, 0, 0}) {
  const auto path = companion(
      executable.c_str(),
      (std::string("data/world/editor-aux/color-channels/") + name + ".png")
          .c_str());
  EditorImages::Info info{};
  if (!EditorRender::inspectImage(path.c_str(), info)) {
    EditorRender::fail("Cannot read original RAW slider skin: " + path);
    return false;
  }
  if (region.w <= 0 || region.h <= 0)
    region = {0, 0, float(info.width), float(info.height)};
  EditorColorPickerArt::ClipGuard guard;
  EditorRender::customImage(
      path, info, region,
      [&](EditorRender::Vec2 p) {
        return EditorColorPickerArt::vertex068(
            {dest.x + (p.x - region.x) * dest.w / region.w,
             dest.y + (p.y - region.y) * dest.h / region.h},0,0);
      },
      0xffffffffu);
  return true;
}
inline void rawStyle(const char *name, Rect r) {
  if (r.w <= 0 || r.h <= 0)
    return;
  // StyleBoxFlat adapts narrow corner radii while retaining its AA width.
  // Nine-slicing the normal corner would also scale that AA fringe.
  if (r.h == 8 && r.w >= 1 && r.w < 8 && r.w == std::floor(r.w) &&
      (!std::strcmp(name, "ChannelRawStyle_grabber_area") ||
       !std::strcmp(name, "ChannelRawStyle_grabber_area_highlight"))) {
    const std::string captured = std::string("ChannelRawNarrow_") +
                                 (name + std::strlen("ChannelRawStyle_")) +
                                 "_" + std::to_string(unsigned(r.w));
    rawImage(captured.c_str(), {r.x - 4, r.y - 4, r.w + 8, r.h + 8});
    return;
  }
  const float radius = std::min({4.f, r.w / 2, r.h / 2}), sx[] = {0, 8, 64, 72},
              sy[] = {0, 8, 32, 40},
              dx[] = {r.x - 4, r.x + radius, r.x + r.w - radius, r.x + r.w + 4},
              dy[] = {r.y - 4, r.y + radius, r.y + r.h - radius, r.y + r.h + 4};
  for (unsigned j = 0; j < 3; ++j)
    for (unsigned i = 0; i < 3; ++i)
      if (dx[i + 1] > dx[i] && dy[j + 1] > dy[j])
        rawImage(name, {dx[i], dy[j], dx[i + 1] - dx[i], dy[j + 1] - dy[j]},
                 {sx[i], sy[j], sx[i + 1] - sx[i], sy[j + 1] - sy[j]});
}
inline void rawSlider(Rect r, float x, float y, double fraction, bool hover) {
  float f = float(std::clamp(fraction, 0., 1.)), p = float(int(r.w * f));
  rawStyle("ChannelRawStyle_slider", {x + r.x, y + r.y + 4, r.w, 8});
  if (p > 0)
    rawStyle(hover ? "ChannelRawStyle_grabber_area_highlight"
                   : "ChannelRawStyle_grabber_area",
             {x + r.x, y + r.y + 4, p, 8});
  rawImage(hover ? "ChannelRawIcon_grabber_highlight"
                 : "ChannelRawIcon_grabber",
           {x + r.x + float(int(r.w * f - 8)), y + r.y, 16, 16});
}
inline std::array<uint32_t, 800> okHue{};
inline float lastS = -100, lastL = -100;
inline void updateOkHue(float s, float l) {
  if (lastS == s && lastL == l)
    return;
  std::array<ok_color::Lab, 7> points{};
  std::array<float, 7> offsets{};
  for (unsigned i = 0; i < 7; ++i) {
    offsets[i] = float(i) / 6;
    points[i] = lab(EditorColorPickerArt::Raster::ok(offsets[i], s, l));
  }
  for (unsigned i = 0; i < 800; ++i) {
    float f = float(i) / 799;
    unsigned a = 0;
    while (a < 5 && f > offsets[a + 1])
      ++a;
    float t = (f - offsets[a]) / (offsets[a + 1] - offsets[a]);
    auto p = points[a], q = points[a + 1];
    okHue[i] = pixel(fromLab(
        {p.L + (q.L - p.L) * t, p.a + (q.a - p.a) * t, p.b + (q.b - p.b) * t}));
  }
  lastS = s;
  lastL = l;
}
inline RGB channel(const Context &ctx, unsigned i, float u) {
  const auto color = ctx.picker.value;
  const auto hsv = EditorColors::toHsv(color);
  const auto mode = ctx.picker.mode;
  if (mode == EditorColors::State::Mode::RGB) {
    auto c = rgb(color);
    if (i == 0)
      c.r = u;
    else if (i == 1)
      c.g = u;
    else
      c.b = u;
    return c;
  }
  if (mode == EditorColors::State::Mode::HSV) {
    float h = float(hsv.h / 360), s = float(hsv.s), v = float(hsv.v);
    if (i == 0) {
      auto c = unpack(
          EditorColorPickerArt::SourceHue[std::min(799u, unsigned(u * 800))]);
      return over({c.r * v, c.g * v, c.b * v}, s, {v, v, v});
    }
    if (i == 1) {
      float hue = std::abs(s) < .00001f ? float(ctx.picker.cachedHue / 360) : h;
      return mix({v, v, v}, EditorColorPickerArt::Raster::hsv(hue, 1, v), u);
    }
    return mix({0, 0, 0}, EditorColorPickerArt::Raster::hsv(h, s, 1), u);
  }
  const auto ok = ctx.picker.ok();
  if (i == 0)
    return unpack(okHue[std::min(799u, unsigned(u * 800))]);
  float h =
      std::abs(ok.s) < .00001f ? float(ctx.picker.cachedOkHue / 360) : ok.h;
  if (i == 1)
    return mix(EditorColorPickerArt::Raster::ok(ok.h, 0, ok.l),
               EditorColorPickerArt::Raster::ok(h, 1, ok.l), u);
  const float s =
      std::abs(ok.l) < .00001f ? float(ctx.picker.cachedOkSaturation) : ok.s;
  auto mid = EditorColorPickerArt::Raster::ok(h, s, .5f),
       right = EditorColorPickerArt::Raster::ok(h, s, 1);
  return u < .5f ? mix({0, 0, 0}, mid, u * 2) : mix(mid, right, (u - .5f) * 2);
}
} // namespace ChannelArt
inline void drawChannels(const EditorColorPickerArt::Layout &m, float x,
                         float y, const Context &ctx, int hover) {
  using namespace ChannelArt;
  if (!checkerLoad())
    return;
  if (ctx.picker.mode == EditorColors::State::Mode::OKHSL) {
    auto ok = ctx.picker.ok();
    updateOkHue(ok.s, ok.l);
  }
  for (unsigned i = 0; i < 4; ++i) {
    const double fraction =
        ctx.picker.channel(i) / std::max(ctx.picker.maximum(i), .00001);
    const auto r = m.slider[i];
    if (ctx.focusChannel == int(i))
      rawStyle("ChannelRawStyle_focus", {x + r.x, y + r.y, r.w, r.h});
    if (i < 3 && ctx.picker.mode == EditorColors::State::Mode::Linear) {
      rawSlider(r, x, y, fraction, hover == int(i));
      continue;
    }
    strip(r, x, y, [&](unsigned xx, unsigned yy) {
      if (!ctx.colorized)
        return uint32_t(0xff372924);
      float u = (xx + .5f) / r.w;
      if (i == 3)
        return pixel(over(rgb(ctx.picker.value), u, unpack(check(xx, yy))));
      return pixel(channel(ctx, i, u));
    });
    EditorColorPickerArt::drawKnob(r, x, y, fraction);
  }
}
inline void drawSample(const EditorColorPickerArt::Rect &r, float x, float y,
                       Color oldColor, Color color) {
  using namespace ChannelArt;
  if (!checkerLoad())
    return;
  strip(r, x, y, [&](unsigned xx, unsigned yy) {
    if (yy + .5f >= r.h * .95f)
      return uint32_t(0);
    const bool old = xx + .5f < r.w / 2;
    const auto c = old ? oldColor : color;
    const unsigned localX = unsigned(std::max(0.f, xx - (old ? 0.f : r.w / 2)));
    return pixel(over(rgb(c), float(c.a), unpack(check(localX, yy))));
  });
  if (!EditorColorPickerArt::same(oldColor, color)) {
    float luminance = float(.2126 * oldColor.r + .7152 * oldColor.g +
                            .0722 * oldColor.b),
          alpha = float(oldColor.a);
    const bool light = .75f + (luminance - .75f) * alpha < .455f;
    EditorColorPickerArt::drawRevert(r, x, y,
                                     light ? 0xffffffffu : 0xff030303u);
  }
  if (oldColor.r > 1 || oldColor.g > 1 || oldColor.b > 1)
    EditorColorPickerArt::image("PickerIcon_overbright_indicator",
                                {x + r.x, y + r.y, 16, 16});
  if (color.r > 1 || color.g > 1 || color.b > 1)
    EditorColorPickerArt::image("PickerIcon_overbright_indicator",
                                {x + r.x + r.w / 2, y + r.y, 16, 16});
}
} // namespace EditorPanelColor
