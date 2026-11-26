#pragma once
#include "editor_font_masks_data.hpp"
#include <climits>
#include <cstdlib>
#include <limits>
#include <malloc.h>
#include <tuple>
namespace EditorRender {
namespace FontMasks {
struct Record {
  uint32_t offset, bytes, raw, crc;
  uint16_t tw, th, w, h;
  float dx, dy, yphase;
  uint8_t phases, flags;
  uint16_t reserved;
};
static_assert(sizeof(Record) == 40, "Font mask index ABI");
// Optional byte windows keep nearby metadata and compressed glyphs separate.
// Four archives * two 1KiB windows = at most 8KiB, allocated after validation.
// Pixel residency/LRU and the exact decoder retain their existing budget/order.
inline constexpr unsigned IoWindowBytes069 = 1024;
struct IoWindow069 { uint32_t offset = 0, bytes = 0; };
struct Archive {
  FILE *file = nullptr;
  uint32_t bytes = 0;
  uint8_t *io = nullptr;
  IoWindow069 indexWindow{}, dataWindow{};
};
struct Entry {
  uint8_t *pixels = nullptr;
  unsigned key = UINT_MAX, age = 0, capacity = 0;
  Record record{};
};
inline std::array<Archive, 4> archives{};
inline std::array<Entry, 512> cache{};
// Hints hold slot numbers only. Keys are always rechecked after eviction/reuse.
inline std::array<uint16_t, 1024> hints068{};
inline unsigned hintBucket068(unsigned key){return (key*2654435761u)>>22;}
inline void remember068(unsigned key,const Entry* p){hints068[hintBucket068(key)]=uint16_t(p-cache.data()+1);}
inline unsigned used = 0, age = 0, reads = 0;
inline constexpr unsigned Limit = 1024 * 1024 - sizeof(cache);
alignas(64) inline const std::array<uint32_t, 256> WhitePalette = []() {
  std::array<uint32_t, 256> a{};
  for (unsigned i = 0; i < 256; ++i)
    a[i] = 0xffffffu | (i << 24);
  return a;
}();
alignas(64) inline const std::array<uint32_t, 256> BlackPalette = []() {
  std::array<uint32_t, 256> a{};
  for (unsigned i = 0; i < 256; ++i)
    a[i] = i << 24;
  return a;
}();
inline unsigned cacheBytes() {
  unsigned bytes = used + sizeof(cache) + sizeof(hints068);
  // Include the new archive metadata as well as every successful allocation.
  bytes += sizeof(archives);
  for (const auto &a : archives) if (a.io) bytes += 2 * IoWindowBytes069;
  return bytes;
}
inline const std::string &getError() { return EditorRender::getError(); }
inline bool error(const std::string &text) {
  EditorRender::fail("Font masks: " + text);
  return false;
}
inline void release() {
  flushBatch();
  for (auto &p : cache) {
    std::free(p.pixels);
    p = Entry{};
  }
  used = 0;
  hints068.fill(0);
  for (auto &a : archives) {
    if (a.file)
      std::fclose(a.file);
    std::free(a.io);
    a = Archive{};
  }
}
inline bool load(Bank bank) {
  auto &a = archives[bank];
  if (a.file)
    return true;
  std::string path = ::companion(executable.c_str(), Paths[bank]);
  a.file = std::fopen(path.c_str(), "rb");
  uint32_t header[4]{};
  if (!a.file || std::fread(header, sizeof header, 1, a.file) != 1 ||
      header[0] != 0x4d464355 || header[1] != 1 || header[2] != Counts[bank] ||
      header[3] != 40) {
    if (a.file)
      std::fclose(a.file);
    a.file = nullptr;
    return error("Cannot read " + path);
  }
  if (std::fseek(a.file, 0, SEEK_END) ||
      std::ftell(a.file) < long(16 + 40 * Counts[bank])) {
    std::fclose(a.file);
    a=Archive{};
    return error("Invalid archive size");
  }
  a.bytes = uint32_t(std::ftell(a.file));
  // Allocation failure is optional: the original direct I/O path remains live.
  a.io = static_cast<uint8_t *>(std::malloc(2 * IoWindowBytes069));
  return true;
}
// Return only complete requested spans. Extra bytes are clipped to the validated
// immutable archive; failed read-ahead is discarded and retried by direct I/O.
inline uint8_t *ioSpan069(Archive &a, bool index, uint32_t offset, unsigned bytes) {
  if (!a.io || !bytes || bytes > IoWindowBytes069 || offset > a.bytes ||
      bytes > a.bytes - offset) return nullptr;
  auto &window = index ? a.indexWindow : a.dataWindow;
  auto *data = a.io + (index ? 0 : IoWindowBytes069);
  if (offset >= window.offset && offset - window.offset <= window.bytes &&
      bytes <= window.bytes - (offset - window.offset))
    return data + (offset - window.offset);
  const uint32_t begin = offset & ~(IoWindowBytes069 - 1u);
  // Full windows begin at a stdio-sized boundary, avoiding a second refill for
  // the unused prefix. Cross-boundary spans keep the original streaming path.
  if (bytes > IoWindowBytes069 - (offset - begin)) return nullptr;
  window.bytes = 0;
  const unsigned wanted = std::min<unsigned>(IoWindowBytes069, a.bytes - begin);
  if (std::fseek(a.file, long(begin), SEEK_SET) ||
      std::fread(data, 1, wanted, a.file) != wanted) {
    std::clearerr(a.file);
    return nullptr;
  }
  window.offset = begin;
  window.bytes = wanted;
  return data + (offset - begin);
}
// Byte windows are expendable under heap pressure; retry pixel allocation
// before the existing eviction policy can discard an otherwise resident glyph.
inline bool dropIo069() {
  bool freed = false;
  for (auto &a : archives) if (a.io) {
    std::free(a.io);
    a.io = nullptr;
    a.indexWindow = IoWindow069{};
    a.dataWindow = IoWindow069{};
    freed = true;
  }
  return freed;
}
inline Entry *oldest(Entry *except = nullptr) {
  Entry *best = nullptr;
  for (auto &p : cache)
    if (&p != except && p.pixels && (!best || p.age < best->age))
      best = &p;
  return best;
}
inline void discard(Entry &p) {
  used -= p.capacity;
  std::free(p.pixels);
  p = Entry{};
}
inline Entry *page(Bank bank, unsigned index) {
  unsigned key = unsigned(bank) * 65536 + index;
  const unsigned hint=hints068[hintBucket068(key)];
  if(hint&&hint<=cache.size()&&cache[hint-1].key==key){auto&p=cache[hint-1];p.age=++age;return &p;}
  for (auto &p : cache)
    if (p.key == key) {
      p.age = ++age;
      remember068(key,&p);
      return &p;
    }
  flushBatch();
  if (index >= Counts[bank] || !load(bank))
    return nullptr;
  auto &a = archives[bank];
  Record record{};
  auto *indexBytes = ioSpan069(a, true, 16 + 40 * index, sizeof record);
  if (indexBytes) std::memcpy(&record, indexBytes, sizeof record);
  else if (std::fseek(a.file, long(16 + 40 * index), SEEK_SET) ||
      std::fread(&record, sizeof record, 1, a.file) != 1) {
    error("Cannot read glyph index");
    return nullptr;
  }
  Entry *target = nullptr;
  for (auto &p : cache)
    if (p.key == UINT_MAX) {
      target = &p;
      break;
    }
  if (!target)
    for(auto&p:cache)if(!target||p.age<target->age)target=&p;
  if (!target) {
    error("No cache entry");
    return nullptr;
  }
  if (!record.raw) {
    if(record.offset||record.bytes||record.tw||record.th||record.w||record.h){error("Invalid empty glyph record");return nullptr;}
    target->key = key;
    target->record = record;
    target->age = ++age;
    remember068(key,target);
    return target;
  }
  if (record.tw < 16 || record.tw > 256 || record.th < 8 || record.th > 256 ||
      (record.tw & (record.tw - 1)) || (record.th & (record.th - 1)) ||
      record.raw != unsigned(record.tw) * record.th || record.w > record.tw ||
      record.h > record.th || record.offset < 16 + 40 * Counts[bank] ||
      uint64_t(record.offset) + record.bytes > a.bytes||!std::isfinite(record.dx)||!std::isfinite(record.dy)||!std::isfinite(record.yphase)||record.yphase<0||record.yphase>=1) {
    error("Invalid glyph record");
    return nullptr;
  }
  if (target->capacity < record.raw) {
    if (target->pixels)
      discard(*target);
    while (used + record.raw > Limit) {
      auto *p = oldest(target);
      if (!p) {
        error("Glyph exceeds cache budget");
        return nullptr;
      }
      discard(*p);
    }
    target->pixels = static_cast<uint8_t *>(memalign(64, record.raw));
    while (!target->pixels) {
      if (dropIo069()) {
        target->pixels = static_cast<uint8_t *>(memalign(64, record.raw));
        if (target->pixels) break;
      }
      auto *p = oldest(target);
      if (!p) {
        error("Cannot allocate exact glyph texture");
        return nullptr;
      }
      discard(*p);
      target->pixels = static_cast<uint8_t *>(memalign(64, record.raw));
    }
    target->capacity = record.raw;
    used += record.raw;
  }
  target->key = UINT_MAX;
  if (!EditorArt::workspace) {
    error("Bounded64KiB workspace unavailable");
    return nullptr;
  }
  auto *encodedBytes = ioSpan069(a, false, record.offset, record.bytes);
  if (!encodedBytes && std::fseek(a.file, long(record.offset), SEEK_SET)) {
    error("Cannot seek glyph texture");
    return nullptr;
  }
  EditorArt::Arena arena{EditorArt::workspace};
  z_stream z{};
  z.zalloc = EditorArt::allocate;
  z.zfree = EditorArt::release;
  z.opaque = &arena;
  if (inflateInit(&z) != Z_OK) {
    error("Cannot initialize bounded decoder");
    return nullptr;
  }
  uint8_t input[2048];
  unsigned left = record.bytes;
  z.next_out = target->pixels;
  z.avail_out = record.raw;
  bool good = false;
  for (;;) {
    if (!z.avail_in && left) {
      unsigned n = std::min<unsigned>(sizeof input, left);
      if (!encodedBytes && std::fread(input, 1, n, a.file) != n)
        break;
      z.next_in = encodedBytes ? encodedBytes + record.bytes - left : input;
      left -= n;
      z.avail_in = n;
    }
    int status = ::inflate(&z, Z_NO_FLUSH);
    if (status == Z_STREAM_END) {
      good = z.total_out == record.raw && !left && !z.avail_in &&
             crc32(0, target->pixels, record.raw) == record.crc;
      break;
    }
    if (status != Z_OK || !z.avail_out || (!left && !z.avail_in))
      break;
  }
  inflateEnd(&z);
  if (!good) {
    error("Glyph decode/CRC failed");
    return nullptr;
  }
  sceKernelDcacheWritebackRange(target->pixels, record.raw);
  markTextureDirty();
  target->key = key;
  target->record = record;
  target->age = ++age;
  ++reads;
  remember068(key,target);
  return target;
}
inline const Map *find(Bank bank, unsigned page, unsigned u, unsigned v,
                       unsigned w, unsigned h) {
  const Map *data = nullptr;
  size_t count = 0;
  switch (bank) {
  case Hud:
    data = HudMaps;
    count = std::size(HudMaps);
    break;
  case Browser:
    data = BrowserMaps;
    count = std::size(BrowserMaps);
    break;
  case Score:
    data = ScoreMaps;
    count = std::size(ScoreMaps);
    break;
  case Legacy:
    data = LegacyMaps;
    count = std::size(LegacyMaps);
    break;
  }
  const auto wanted = std::make_tuple(page, u, v, w, h);
  size_t lo = 0, hi = count;
  while (lo < hi) {
    size_t mid = (lo + hi) / 2;
    const auto &m = data[mid];
    auto value = std::make_tuple(unsigned(m.page), unsigned(m.u), unsigned(m.v),
                                 unsigned(m.w), unsigned(m.h));
    if (value < wanted)
      lo = mid + 1;
    else
      hi = mid;
  }
  if (lo < count) {
    const auto &m = data[lo];
    if (std::make_tuple(unsigned(m.page), unsigned(m.u), unsigned(m.v),
                        unsigned(m.w), unsigned(m.h)) == wanted)
      return &m;
  }
  return nullptr;
}
inline bool
drawPart(Bank bank, unsigned atlasPage, Layer layer, const std::array<Vertex, 4> &q,
     uint32_t tint = White, const View *view = nullptr,
     const Color *rawTint = nullptr, float localPen = 0,
     float localBaseline = std::numeric_limits<float>::quiet_NaN(),bool shadowOutline=false) {
  if (!visible(q))
    return true;
  unsigned u = unsigned(q[0].u), v = unsigned(q[0].v),
           w = unsigned(q[1].u - q[0].u), h = unsigned(q[3].v - q[0].v);
  const auto *m = find(bank, atlasPage, u, v, w, h);
  if (!m)
    return error("Unknown frozen glyph UV");
  float bias = m->phases > 1 ? .5f / m->phases : 0,
        at = float(std::floor(m->phases * (localPen + bias)) -
                   m->phases * std::floor(localPen + bias));
  unsigned phase = unsigned(at) * (4 / m->phases);
  auto *p = page(bank, m->base + (shadowOutline?12:unsigned(layer)*4) + phase);
  if (!p)
    return false;
  const auto &r = p->record;
  if (!r.w || !r.h)
    return true;
  float dx = r.dx, dy = r.dy;
  if (!(m->flags & 1)) {
    dx += std::floor(localPen + bias) - localPen + float(phase) * .25f;
    if (std::isfinite(localBaseline))
      dy += std::floor(localBaseline) - localBaseline + r.yphase;
  }
  Vertex base = q[0], xAxis{(q[1].x - base.x) / w, (q[1].y - base.y) / w, 0, 0},
         yAxis{(q[3].x - base.x) / h, (q[3].y - base.y) / h, 0, 0};
  std::array<Vertex, 4> quad{};
  float xs[] = {dx, dx + r.w, dx + r.w, dx},
        ys[] = {dy, dy, dy + r.h, dy + r.h},
        us[] = {0, float(r.w), float(r.w), 0},
        vs[] = {0, 0, float(r.h), float(r.h)};
  for (unsigned i = 0; i < 4; ++i)
    quad[i] = {base.x + xAxis.x * xs[i] + yAxis.x * ys[i],
               base.y + xAxis.y * xs[i] + yAxis.y * ys[i], us[i], vs[i]};
  const auto *palette = (layer == Fill ? WhitePalette : BlackPalette).data();
  if (view && (!Lighting::points.empty() || highColor(rawTint) ||
               highColor(&view->light)))
    shadeTexture(*view, p->pixels, GU_PSM_T8, r.tw, r.th, quad, tint, palette,
                 256, rawTint);
  else
    polygonTexture(p->pixels, GU_PSM_T8, r.tw, r.th, quad.data(), 4,
                   ambientTint(tint, view), palette, 256);
  return true;
}
inline bool draw(Bank bank,unsigned atlasPage,Layer layer,const std::array<Vertex,4>&q,uint32_t tint=White,const View*view=nullptr,const Color*rawTint=nullptr,float localPen=0,float localBaseline=std::numeric_limits<float>::quiet_NaN()){
 if(!drawPart(bank,atlasPage,layer,q,tint,view,rawTint,localPen,localBaseline))return false;
 if(layer==Shadow){const auto*m=find(bank,atlasPage,unsigned(q[0].u),unsigned(q[0].v),unsigned(q[1].u-q[0].u),unsigned(q[3].v-q[0].v));if(m&&(m->flags&2))return drawPart(bank,atlasPage,layer,q,tint,view,rawTint,localPen,localBaseline,true);}
 return true;
}
} // namespace FontMasks
} // namespace EditorRender
namespace EditorRender {
inline void releaseFontMasks() { FontMasks::release(); }
} // namespace EditorRender
