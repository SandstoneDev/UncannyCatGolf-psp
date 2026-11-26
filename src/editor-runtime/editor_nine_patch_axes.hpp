#pragma once
#include <climits>
#include <cmath>
#include <cstdint>
namespace EditorNinePatchAxes {
struct UV {
  float region = 0, extent = 32;
  unsigned near = 4, far = 4, mode = 1;
};
struct Input {
  float cameraX = 0, cameraY = 0, centerX = 384, centerY = 256, step = 1,
        positionX = 0, positionY = 0, inverseX = 1, inverseY = 1, ca = 1,
        sa = 0, pixelScale = 17.f / 32;
  int bx = 36, by = 0, viewX = 36;
  unsigned w = 256, h = 256;
  double width = 256, height = 256;
  UV x, y;
};
struct Axes {
  float wx[256]{}, wy[256]{};
  int u[256]{}, v[256]{};
  uint8_t validX[256]{}, validY[256]{};
  unsigned w = 0, h = 0;
};
static_assert(sizeof(Axes) <= 5120, "NinePatch axes stack budget");
// This is deliberately the original DOUBLE lambda, including operation order.
inline double uv(double p, double size, unsigned near, unsigned far,
                 double extent, unsigned mode) {
  if (p < near)
    return p;
  if (p >= size - far)
    return extent - (size - p);
  double source = extent - near - far, target = size - near - far;
  return near + (mode == 1 ? std::fmod(p - near, source)
                           : (p - near) / target * source);
}
inline bool validUV(const UV &mapping, double size) {
  return std::isfinite(mapping.region) && std::isfinite(mapping.extent) &&
         mapping.mode <= 1 &&
         double(mapping.extent) - mapping.near - mapping.far > 0 &&
         size - mapping.near - mapping.far > 0;
}
inline bool texel(const UV &mapping, double size, double p, int &out) {
  const double at =
      std::floor(mapping.region + uv(p, size, mapping.near, mapping.far,
                                     mapping.extent, mapping.mode));
  if (!std::isfinite(at) || at < INT_MIN || at > INT_MAX)
    return false;
  out = int(at);
  return true;
}
// On false, ignore all arrays (w/h are zero). No allocations and no persistent
// state. ca==1 and sa==+/-0 ONLY. Keep all original float cross-term
// operations; a strictly positive finite half extent absorbs any sign-of-zero
// difference caused by choosing one reference row/column. Nonzero cross terms
// are refused.
inline bool build(const Input &in, Axes &out) {
  out.w = out.h = 0;
  if (!in.w || !in.h || in.w > 256 || in.h > 256 || in.ca != 1 || in.sa != 0 ||
      !std::isfinite(in.cameraX) || !std::isfinite(in.cameraY) ||
      !std::isfinite(in.centerX) || !std::isfinite(in.centerY) ||
      !std::isfinite(in.positionX) || !std::isfinite(in.positionY) ||
      !std::isfinite(in.step) || in.step == 0 || !std::isfinite(in.inverseX) ||
      !std::isfinite(in.inverseY) || in.inverseX == 0 || in.inverseY == 0 ||
      !std::isfinite(in.pixelScale) || in.pixelScale <= 0 ||
      !std::isfinite(in.width) || !std::isfinite(in.height) ||
      !validUV(in.x, in.width) || !validUV(in.y, in.height))
    return false;
  const float halfWidth = float(in.width) * .5f,
              halfHeight = float(in.height) * .5f;
  if (!std::isfinite(halfWidth) || !std::isfinite(halfHeight) ||
      halfWidth <= 0 || halfHeight <= 0)
    return false;
  // The unsigned zero is intentional: bx+x/by+y in the source promote the
  // signed physical tile origin to unsigned before conversion to float.
  const float wx0 = in.cameraX +
                    (in.bx + 0u + .5f - in.viewX - in.centerX * in.pixelScale) *
                        in.step,
              wy0 = in.cameraY +
                    (in.by + 0u + .5f - in.centerY * in.pixelScale) * in.step,
              dx0 = wx0 - in.positionX, dy0 = wy0 - in.positionY;
  if (!std::isfinite(dx0) || !std::isfinite(dy0))
    return false;
  for (unsigned x = 0; x < in.w; ++x) {
    const float wx = in.cameraX +
                     (in.bx + x + .5f - in.viewX - in.centerX * in.pixelScale) *
                         in.step,
                dx = wx - in.positionX;
    if (!std::isfinite(wx) || !std::isfinite(dx))
      return false;
    const double p =
        (dx * in.ca + dy0 * in.sa) * in.inverseX + float(in.width) * .5f;
    if (!std::isfinite(p))
      return false;
    out.wx[x] = wx;
    out.validX[x] = uint8_t(!(p < 0 || p >= in.width));
    out.u[x] = 0;
    if (out.validX[x] && !texel(in.x, in.width, p, out.u[x]))
      return false;
  }
  for (unsigned y = 0; y < in.h; ++y) {
    const float wy = in.cameraY +
                     (in.by + y + .5f - in.centerY * in.pixelScale) * in.step,
                dy = wy - in.positionY;
    if (!std::isfinite(wy) || !std::isfinite(dy))
      return false;
    const double p =
        (-dx0 * in.sa + dy * in.ca) * in.inverseY + float(in.height) * .5f;
    if (!std::isfinite(p))
      return false;
    out.wy[y] = wy;
    out.validY[y] = uint8_t(!(p < 0 || p >= in.height));
    out.v[y] = 0;
    if (out.validY[y] && !texel(in.y, in.height, p, out.v[y]))
      return false;
  }
  out.w = in.w;
  out.h = in.h;
  return true;
}
} // namespace EditorNinePatchAxes
