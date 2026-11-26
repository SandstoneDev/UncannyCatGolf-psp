#pragma once
#include "editor_ok_color.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace EditorColorPickerArt {
namespace Raster {
inline constexpr float Tau = 6.2831853071795864769f;
struct RGB {
  float r, g, b;
};
inline RGB hsv(float h, float s, float v) {
  h -= std::floor(h);
  float q = h * 6;
  int sector = int(q);
  float t = q - sector;
  float low = v * (1 - s), down = v * (1 - s * t), up = v * (1 - s * (1 - t));
  switch (sector % 6) {
  case 0:
    return {v, up, low};
  case 1:
    return {down, v, low};
  case 2:
    return {low, v, up};
  case 3:
    return {low, down, v};
  case 4:
    return {up, low, v};
  default:
    return {v, low, down};
  }
}
inline uint32_t rgba(RGB c, float a = 1) {
  auto b = [](float v) {
    return uint32_t(std::clamp(std::lround(v * 255.f), 0l, 255l));
  };
  return b(c.r) | (b(c.g) << 8) | (b(c.b) << 16) | (b(a) << 24);
}
inline RGB ok(float h, float s, float l) {
  h -= std::floor(h);
  auto c = ok_color::okhsl_to_srgb({h, s, l});
  return {c.r, c.g, c.b};
}
struct Sample {
  RGB color;
  float alpha;
};
inline Sample sample(unsigned shape, float width, float height, float px,
                     float py, float hue, float value, float light) {
  const float ux = px / width, uy = py / height;
  if (!shape)
    return {hsv(hue, ux, 1 - uy), 1};
  const float dx = ux - .5f, dy = uy - .5f;
  if (shape == 1 && std::abs(dx) <= .2969848481f &&
      std::abs(dy) <= .2969848481f)
    return {hsv(hue, (dx + .2969848481f) / .5939696962f,
                (.2969848481f - dy) / .5939696962f),
            1};
  float alpha = 0;
  for (unsigned i = 0; i < 4; ++i) {
    float x = dx + ((i == 0 || i == 3) ? .001f : -.001f),
          y = dy + (i < 2 ? .001f : -.001f), r2 = x * x + y * y;
    if (r2 < .25f && (shape != 1 || r2 > .42f * .42f))
      alpha += .25f;
  }
  const float h = std::atan2(dy, dx) / Tau;
  if (shape == 1)
    return {hsv(h, 1, 1), alpha};
  if (shape == 2)
    return {hsv(h, std::hypot(dx + .001f, dy - .001f) * 2, value), alpha};
  return {ok(h, std::hypot(dx, dy) * 2, light), alpha};
}
inline uint32_t pixel(unsigned shape, float w, float h, float x, float y,
                      float hue, float v, float l) {
  auto p = sample(shape, w, h, x, y, hue, v, l);
  return rgba(p.color, p.alpha);
}
inline uint32_t bar(unsigned shape, float fraction, float hue,
                    float saturation) {
  if (!shape)
    return rgba(hsv(fraction, 1, 1));
  if (shape == 2)
    return rgba(hsv(hue, saturation, 1 - fraction));
  auto c = ok(hue, saturation, .5f);
  float f = fraction * 2;
  if (f <= 1)
    return rgba({1 + (c.r - 1) * f, 1 + (c.g - 1) * f, 1 + (c.b - 1) * f});
  f = 2 - f;
  return rgba({c.r * f, c.g * f, c.b * f});
}
} // namespace Raster
} // namespace EditorColorPickerArt
