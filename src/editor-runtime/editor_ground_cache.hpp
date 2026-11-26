#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include "editor_ground_proof_layout.hpp"

namespace EditorGroundCache {
inline constexpr unsigned VramBytes = 0x200000, FrameBytes = 0x44000,
                          FrameStride = 512;
inline constexpr unsigned Tail = 0x1d8000, PixelBytes = 147968,
                          End = Tail + PixelBytes;
inline constexpr unsigned KeyBytes = 4096, MaxLights = 16, MaxNodes = 16,
                          MaxVertices = 64;
struct Page {
  unsigned offset, x, y, w, h;
};
inline constexpr std::array<Page, 4> Pages{{{0x1d8000, 0, 0, 256, 256},
                                            {0x1f8000, 256, 0, 16, 256},
                                            {0x1fa000, 0, 256, 256, 16},
                                            {0x1fc000, 256, 256, 16, 16}}};
static_assert(End == 0x1fc200 && End <= VramBytes,
              "unused Module VRAM tail only");
struct Stamp {
  uint64_t scene = 0, document = 0, assets = 0;
  bool operator==(const Stamp &b) const {
    return scene == b.scene && document == b.document && assets == b.assets;
  }
};
struct Color {
  double r = 1, g = 1, b = 1, a = 1;
};
struct Point {
  float x = 0, y = 0;
};
struct Light {
  float x = 0, y = 0, xx = 0, xy = 0, yx = 0, yy = 0, r = 0, g = 0, b = 0,
        a = 0, extentX = 0, extentY = 0;
};
struct View {
  double cameraX = 0, cameraY = 0, centerX = 384, centerY = 256, zoom = 1,
         inverseZoom = 1, offsetX = 0, offsetY = 0;
  float clipX = 172, clipY = 0, clipRight = 444, clipBottom = 272;
  float floorStartX = 0, floorStartY = 0, floorStep = 1;
};
struct Palette {
  std::array<Color, 4> source{};
  std::array<uint32_t, 4> rgba{};
};
struct Node {
  unsigned order = 0, id = 19;
  int variant = 0;
  float x = 0, y = 0, sx = 1, sy = 1, rotation = 0;
  Color tint{};
  unsigned first = 0, count = 0;
};
struct State {
  Stamp stamp{};
  View view{};
  Color ambientSource{};
  std::array<float, 4> ambientShader{{1, 1, 1, 1}};
  uint32_t ambientRgba = 0xffffffffu;
  std::array<Light, MaxLights> lights{};
  unsigned lightCount = 0;
  std::array<unsigned,16> incomingActive{};
  unsigned incomingCount = 0;
  std::array<double, 6> layerAlpha{{1, 1, 1, 1, 1, 1}};
  std::array<Palette, 6> palettes{};
  std::array<Node, MaxNodes> nodes{};
  unsigned nodeCount = 0;
  std::array<Point, MaxVertices> vertices{};
  unsigned vertexCount = 0;
  unsigned floorIndex = 3, floorWidth = 513, floorHeight = 513;
  bool play = false, immutableInputs = false, asphaltOpaque = false;
  bool customFloor = false, customTiles = false, water = false,
       behindDecoration = false, liveTrails = false;
  bool knownClear = false, defaultBlend = false, depthOff = false,
       ditherOff = false;
  uint32_t clearRgba = 0xff000000u;
};
struct Key {
  std::array<uint8_t, KeyBytes> bytes{};
  unsigned size = 0;
  Stamp stamp{};
  bool operator==(const Key &b) const {
    return size == b.size && stamp == b.stamp && size <= KeyBytes &&
           !std::memcmp(bytes.data(), b.bytes.data(), size);
  }
};
enum class Eligibility {
  Eligible,
  Capability,
  Nonfinite,
  Capacity,
  Coverage,
  Encoding
};
namespace Detail {
struct Encoder {
  Key &key;
  bool good = true;
  void integer(uint64_t v, unsigned n) {
    if (n > KeyBytes - key.size) {
      good = false;
      return;
    }
    for (unsigned i = 0; i < n; ++i)
      key.bytes[key.size++] = uint8_t(v >> (i * 8));
  }
  void u32(uint32_t v) { integer(v, 4); }
  void u64(uint64_t v) { integer(v, 8); }
  bool scalar(float v) {
    if (!std::isfinite(v))
      return false;
    uint32_t b;
    std::memcpy(&b, &v, 4);
    u32(b);
    return good;
  }
  bool scalar(double v) {
    if (!std::isfinite(v))
      return false;
    uint64_t b;
    std::memcpy(&b, &v, 8);
    u64(b);
    return good;
  }
  bool color(const Color &c) {
    return scalar(c.r) && scalar(c.g) && scalar(c.b) && scalar(c.a);
  }
};
} // namespace Detail
inline Eligibility buildKey(const State &s, Key &out) {
  // Reject unsupported cases. The existing renderer remains the exact fallback.
  if (!s.play || !s.immutableInputs || !s.asphaltOpaque || s.customFloor ||
      s.customTiles || s.water || s.behindDecoration || s.liveTrails ||
      !s.knownClear || !s.defaultBlend || !s.depthOff || !s.ditherOff ||
      s.floorIndex != 3 || s.floorWidth != 513 || s.floorHeight != 513 ||
      s.clearRgba != 0xff000000u || !s.stamp.scene || !s.stamp.document ||
      !s.stamp.assets || s.view.clipX != 172 || s.view.clipY != 0 ||
      s.view.clipRight != 444 || s.view.clipBottom != 272 ||
      s.ambientSource.a != 1.0 || s.ambientShader[3] != 1.f ||
      (s.ambientRgba >> 24) != 255)
    return Eligibility::Capability;
  if (s.lightCount > MaxLights || s.nodeCount > MaxNodes ||
      s.vertexCount > MaxVertices)
    return Eligibility::Capacity;
  if (!(s.view.zoom > 0) || !(s.view.inverseZoom > 0) ||
      !(s.view.floorStep > 0))
    return Eligibility::Nonfinite;
  const float lastX = s.view.floorStartX + 271 * s.view.floorStep,
              lastY = s.view.floorStartY + 271 * s.view.floorStep;
  if (!std::isfinite(lastX) || !std::isfinite(lastY) ||
      s.view.floorStartX < -43256.f || s.view.floorStartY < -43256.f ||
      lastX >= 44903.8f || lastY >= 44903.8f)
    return Eligibility::Coverage;
  Key candidate;
  candidate.stamp = s.stamp;
  Detail::Encoder e{candidate};
  e.u32(2);
  e.u64(s.stamp.scene);
  e.u64(s.stamp.document);
  e.u64(s.stamp.assets);
  e.u32(s.floorIndex);
  e.u32(s.floorWidth);
  e.u32(s.floorHeight);
  e.u32(s.clearRgba);
  const auto &v = s.view;
  for (double x : {v.cameraX, v.cameraY, v.centerX, v.centerY, v.zoom,
                   v.inverseZoom, v.offsetX, v.offsetY})
    if (!e.scalar(x))
      return Eligibility::Nonfinite;
  for (float x : {v.clipX, v.clipY, v.clipRight, v.clipBottom, v.floorStartX,
                  v.floorStartY, v.floorStep})
    if (!e.scalar(x))
      return Eligibility::Nonfinite;
  if (!e.color(s.ambientSource))
    return Eligibility::Nonfinite;
  for (float x : s.ambientShader)
    if (!e.scalar(x))
      return Eligibility::Nonfinite;
  e.u32(s.ambientRgba);
  e.u32(s.incomingCount);
  if(s.incomingCount>16)return Eligibility::Capacity;
  for(unsigned i=0;i<s.incomingCount;++i)e.u32(s.incomingActive[i]);
  e.u32(s.lightCount);
  for (unsigned i = 0; i < s.lightCount; ++i) {
    const auto &p = s.lights[i];
    for (float x : {p.x, p.y, p.xx, p.xy, p.yx, p.yy, p.r, p.g, p.b, p.a,
                    p.extentX, p.extentY})
      if (!e.scalar(x))
        return Eligibility::Nonfinite;
  }
  for (unsigned layer = 0; layer < 6; ++layer) {
    if (!e.scalar(s.layerAlpha[layer]))
      return Eligibility::Nonfinite;
    for (unsigned i = 0; i < 4; ++i) {
      if (!e.color(s.palettes[layer].source[i]))
        return Eligibility::Nonfinite;
      e.u32(s.palettes[layer].rgba[i]);
    }
  }
  e.u32(s.nodeCount);
  e.u32(s.vertexCount);
  unsigned totalVertices = 0;
  for (unsigned i = 0; i < s.nodeCount; ++i) {
    const auto &n = s.nodes[i];
    if ((n.id != 19 && n.id != 20) || n.count < 3 || n.first > s.vertexCount ||
        n.count > s.vertexCount - n.first)
      return Eligibility::Capability;
    if (n.count > MaxVertices - totalVertices)
      return Eligibility::Capacity;
    totalVertices += n.count;
    e.u32(n.order);
    e.u32(n.id);
    e.u32(uint32_t(n.variant));
    e.u32(n.count);
    for (float x : {n.x, n.y, n.sx, n.sy, n.rotation})
      if (!e.scalar(x))
        return Eligibility::Nonfinite;
    if (!e.color(n.tint))
      return Eligibility::Nonfinite;
    for (unsigned j = 0; j < n.count; ++j) {
      const auto &p = s.vertices[n.first + j];
      if (!e.scalar(p.x) || !e.scalar(p.y))
        return Eligibility::Nonfinite;
    }
  }
  if (!e.good)
    return Eligibility::Encoding;
  out = candidate;
  return Eligibility::Eligible;
}
struct Graphics {
  unsigned drawOffset = 0;
  bool fenceOK = true, contextReady = false;
};
struct Transfer {
  unsigned source, sourceStride, sx, sy, destination, destinationStride, dx, dy,
      w, h;
};
inline bool validTransfer(const Transfer &t) {
  if (!t.w || !t.h || !t.sourceStride || !t.destinationStride ||
      t.sourceStride % 16 || t.destinationStride % 16 ||
      t.sx > t.sourceStride || t.w > t.sourceStride - t.sx ||
      t.dx > t.destinationStride || t.w > t.destinationStride - t.dx)
    return false;
  const uint64_t a = uint64_t(t.source) +
                     2 * ((uint64_t(t.sy) + t.h - 1) * t.sourceStride + t.sx +
                          t.w),
                 b = uint64_t(t.destination) +
                     2 * ((uint64_t(t.dy) + t.h - 1) * t.destinationStride +
                          t.dx + t.w);
  return a <= VramBytes && b <= VramBytes;
}
struct Backend {
  bool (*fence)(void *) =
      nullptr; // Submit+finish+sync; restore an open world list/clip.
  bool (*copy)(void *, const Transfer &) =
      nullptr; // Queue RGB565 CopyImage, never texture sampling.
  void *context = nullptr;
};
struct Stats {
  uint64_t hits = 0, misses = 0, captures = 0, published = 0, invalidations = 0,
           copyFailures = 0, fenceFailures = 0, cancelled = 0;
};
enum class Restore { Miss, Hit, RetryOriginal, BackendFault };
class Cache {
  Key key{};
  uint64_t generation = 1, captureGeneration = 0;
  bool valid = false, pending = false, busy = false;
  Stats counters{};
  struct Guard {
    bool &flag;
    explicit Guard(bool &v) : flag(v) { flag = true; }
    ~Guard() { flag = false; }
  };
  static bool drawOffset(unsigned offset) {
    return offset == 0 || offset == FrameBytes;
  }
  bool copies(unsigned offset, bool restore, const Backend &backend) {
    for (const auto &p : Pages) {
      const Transfer t =
          restore ? Transfer{p.offset,    p.w,       0,   0,   offset,
                             FrameStride, 172 + p.x, p.y, p.w, p.h}
                  : Transfer{offset, FrameStride, 172 + p.x, p.y, p.offset,
                             p.w,    0,           0,         p.w, p.h};
      if (!validTransfer(t) || !backend.copy(backend.context, t)) {
        ++counters.copyFailures;
        return false;
      }
    }
    return true;
  }

public:
  static constexpr size_t fixedBytes() { return sizeof(Cache); }
  // Charge persistent metadata and maximum caller State+Key scratch once to the
  // shared8MiB budget. No pixel buffers or hidden heap allocations exist here.
  static constexpr size_t accountedBytes() {
    return sizeof(Cache) + sizeof(State) + 2 * sizeof(Key);
  }
  const Stats &stats() const { return counters; }
#ifdef UCG_EDITOR_CAPTURE
  const Key&diagnosticKey()const{return key;}
#endif
  bool ready() const { return valid; }
  bool awaitingPublication() const { return pending; }
  void invalidate() {
    valid = pending = false;
    if (!++generation)
      ++generation;
    ++counters.invalidations;
  }
  Restore restore(const Key &requested, unsigned offset,
                  const Backend &backend) {
    if (busy || !valid || pending || !(key == requested)) {
      ++counters.misses;
      return Restore::Miss;
    }
    if (!drawOffset(offset) || !backend.fence || !backend.copy) {
      invalidate();
      return Restore::BackendFault;
    }
    Guard guard(busy);
    const uint64_t started = generation;
    if (!backend.fence(backend.context)) {
      ++counters.fenceFailures;
      invalidate();
      return Restore::BackendFault;
    }
    if (started != generation || !valid) {
      ++counters.cancelled;
      return Restore::RetryOriginal;
    }
    const bool copied = copies(offset, true, backend);
    // Always drain a partially queued transfer before the opaque original
    // redraw.
    if (!backend.fence(backend.context)) {
      ++counters.fenceFailures;
      invalidate();
      return Restore::BackendFault;
    }
    if (!copied || started != generation) {
      if (started != generation)
        ++counters.cancelled;
      invalidate();
      return Restore::RetryOriginal;
    }
    ++counters.hits;
    return Restore::Hit;
  }
  bool capture(const Key &requested, unsigned offset, const Backend &backend) {
    if (busy)
      return false;
    Guard guard(busy);
    valid = pending = false;
    if (requested.size > KeyBytes || !requested.size || !drawOffset(offset) ||
        !backend.fence || !backend.copy)
      return false;
    const uint64_t started = generation;
    if (!backend.fence(backend.context)) {
      ++counters.fenceFailures;
      return false;
    }
    const bool copied = copies(offset, false, backend);
    if (!backend.fence(backend.context)) {
      ++counters.fenceFailures;
      return false;
    }
    if (!copied || started != generation) {
      if (started != generation)
        ++counters.cancelled;
      return false;
    }
    key = requested;
    captureGeneration = generation;
    pending = true;
    ++counters.captures;
    return true;
  }
  bool finishFrame(bool fenced, const Stamp &current) {
    if (busy || !pending)
      return false;
    pending = false;
    if (!fenced || captureGeneration != generation || !(key.stamp == current)) {
      valid = false;
      ++counters.cancelled;
      return false;
    }
    valid = true;
    ++counters.published;
    return true;
  }
};
} // namespace EditorGroundCache
