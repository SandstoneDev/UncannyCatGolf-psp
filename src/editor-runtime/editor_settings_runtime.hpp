#pragma once
#include "editor_art.hpp"
#include "editor_settings_data.hpp"
#include <climits>
#include <malloc.h>
#include "editor_settings_main_ui068.hpp"
namespace EditorSettingsArt {
struct Record {
  uint32_t offset, bytes, raw, crc;
};
struct Page {
  uint8_t *pixels = nullptr;
  unsigned record = UINT_MAX, age = 0;
};
inline FILE *archive = nullptr;
inline std::array<Record, Packed::Pages> records{};
inline std::array<Page, 8> pages{};
inline unsigned age = 0, reads = 0;
inline std::string error;
inline const std::string &getError() { return error; }
inline bool fail(const std::string &message) {
  if (error.empty())
    error = message;
  return false;
}
inline void unload() {
  flushBatch();
  EditorSettingsMainUi068::unload();
  if (archive)
    std::fclose(archive);
  archive = nullptr;
  for (auto &p : pages) {
    std::free(p.pixels);
    p = Page{};
  }
  error.clear();
}
inline bool load(const char *exe) {
  if (archive)
    return true;
  error.clear();
  auto file = companion(exe, "data/world/editor-settings.bin");
  archive = std::fopen(file.c_str(), "rb");
  uint32_t h[4]{};
  if (!archive || std::fread(h, sizeof h, 1, archive) != 1 ||
      h[0] != 0x53454355 || h[1] != 1 || h[2] != Packed::Pages ||
      std::fread(records.data(), sizeof records, 1, archive) != 1) {
    if (archive)
      std::fclose(archive);
    archive = nullptr;
    return fail("Cannot read original settings artwork: " + file);
  }
  if (std::fseek(archive, 0, SEEK_END)) {
    unload();
    return fail("Cannot seek original settings artwork");
  }
  long bytes = std::ftell(archive);
  uint64_t next = 16 + sizeof records;
  for (const auto &r : records) {
    if (r.offset != next || r.raw != 262144 ||
        uint64_t(r.offset) + r.bytes > uint64_t(bytes)) {
      unload();
      return fail("Invalid original settings record index");
    }
    next += r.bytes;
  }
  return true;
}
inline bool inflate(unsigned id, uint8_t *target) {
  if (!EditorArt::workspace)
    return fail("Settings requires the bounded editor inflate workspace");
  const auto &r = records[id];
  if (std::fseek(archive, long(r.offset), SEEK_SET))
    return fail("Cannot seek original settings page");
  EditorArt::Arena arena{EditorArt::workspace};
  z_stream z{};
  z.zalloc = EditorArt::allocate;
  z.zfree = EditorArt::release;
  z.opaque = &arena;
  if (inflateInit(&z) != Z_OK)
    return fail("Cannot initialize bounded settings decoder");
  alignas(64) uint8_t input[4096];
  unsigned left = r.bytes;
  z.next_out = target;
  z.avail_out = r.raw;
  bool good = false;
  for (;;) {
    if (!z.avail_in && left) {
      unsigned n = std::min<unsigned>(sizeof input, left);
      if (std::fread(input, 1, n, archive) != n)
        break;
      left -= n;
      z.next_in = input;
      z.avail_in = n;
    }
    int status = ::inflate(&z, Z_NO_FLUSH);
    if (status == Z_STREAM_END) {
      good = z.total_out == r.raw && !left && !z.avail_in &&
             crc32(0, target, r.raw) == r.crc;
      break;
    }
    if (status != Z_OK || !z.avail_out || (!left && !z.avail_in))
      break;
  }
  inflateEnd(&z);
  if (!good)
    return fail("Original settings page failed decode/CRC: " +
                std::to_string(id));
  sceKernelDcacheWritebackRange(target, r.raw);
  ++reads;
  return true;
}
inline const uint8_t *page(unsigned id) {
  for (auto &p : pages)
    if (p.pixels && p.record == id) {
      p.age = ++age;
      return p.pixels;
    }
  flushBatch();
  Page *target = &pages[0];
  for (auto &p : pages)
    if (!p.pixels) {
      target = &p;
      break;
    } else if (p.age < target->age)
      target = &p;
  if (!target->pixels)
    target->pixels = static_cast<uint8_t *>(memalign(64, 262144));
  if (!target->pixels) {
    Page *allocated = nullptr;
    for (auto &p : pages)
      if (p.pixels && (!allocated || p.age < allocated->age))
        allocated = &p;
    if (allocated)
      target = allocated;
  }
  if (!target->pixels) {
    fail("Cannot allocate an exact settings texture page");
    return nullptr;
  }
  target->record = UINT_MAX;
  if (!inflate(id, target->pixels))
    return nullptr;
  target->record = id;
  target->age = ++age;
  return target->pixels;
}
inline void frame(unsigned id, float x = 0, float y = 0) {
  if (id >= std::size(Packed::Frames))
    return;
  const auto &f = Packed::Frames[id];
  for (unsigned i = 0; i < f.count; ++i) {
    const auto &p = Packed::Parts[f.first + i];
    const auto *memory = page(p.page);
    if (!memory)
      continue;
    if (!drawOpen)
      beginBatch();
    sceGuTexFlush();
    setTexture(GU_PSM_8888, 256, 256, memory);
    sceGuTexFilter(GU_NEAREST, GU_NEAREST);
    auto *v =
        static_cast<TextureVertex *>(sceGuGetMemory(2 * sizeof(TextureVertex)));
    const float l = ViewX + x * EditorArt::Scale + p.x,
                t = y * EditorArt::Scale + p.y;
    v[0] = {float(p.u), float(p.v), 0xffffffffu, l, t, 0};
    v[1] = {float(p.u + p.w), float(p.v + p.h), 0xffffffffu,
            l + p.w,          t + p.h,          0};
    sceGuDrawArray(GU_SPRITES,
                   GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF |
                       GU_TRANSFORM_2D,
                   2, nullptr, v);
  }
}
inline void browserBackScale068(unsigned id,float x,float y){
 const auto&f=Packed::Frames[id];constexpr float zoom=1.7f;
 for(unsigned i=0;i<f.count;++i){const auto&p=Packed::Parts[f.first+i];const auto*memory=page(p.page);if(!memory)continue;if(!drawOpen)beginBatch();
 sceGuTexFlush();setTexture(GU_PSM_8888,256,256,memory);sceGuTexFilter(GU_NEAREST,GU_NEAREST);
 auto*v=static_cast<TextureVertex*>(sceGuGetMemory(2*sizeof(TextureVertex)));const float l=ViewX+x*EditorArt::Scale+p.x*zoom,t=y*EditorArt::Scale+p.y*zoom;
 v[0]={float(p.u),float(p.v),White,l,t,0};v[1]={float(p.u+p.w),float(p.v+p.h),White,l+p.w*zoom,t+p.h*zoom,0};
 sceGuDrawArray(GU_SPRITES,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,2,nullptr,v);
 }
}
inline void percent(unsigned lang, unsigned which, double value) {
  char text[24];
  std::snprintf(text, sizeof text, "%d%%", int(value * 100));
  float width = 0;
  for (const char *s = text; *s; ++s)
    for (const auto &g : Packed::Percent[lang])
      if (g.code == unsigned(*s))
        width += g.advance;
  const auto &r = Packed::PercentRects[lang][which];
  float x = r.x + r.w - std::ceil(width), y = r.y + Packed::PercentAscent[lang];
  for (const char *s = text; *s; ++s)
    for (const auto &g : Packed::Percent[lang])
      if (g.code == unsigned(*s)) {
        frame(g.frame, x, y);
        x += g.advance;
        break;
      }
}
inline void slider(unsigned lang, Widget widget, double value, double maximum,
                   unsigned state) {
  const auto &r = Rects[lang][widget];
  frame(Packed::Widgets[lang][widget][state * 2], r.x, r.y);
  double fraction = std::clamp(value / maximum, 0., 1.);
  float center = r.x + float(int((r.w - 16) * fraction + 8));
  // Godot4.4 Slider draws the original StyleBoxTexture into a variable-width
  // nine-patch. Clipping a full filled slider would duplicate its base track.
  const auto &areaFrame =
      Packed::Frames[Packed::AreaFrames[state == 1 || state == 2]];
  const auto &area = Packed::Parts[areaFrame.first];
  const auto *pixels = page(area.page);
  if (pixels) {
    if (!drawOpen)
      beginBatch();
    sceGuTexFlush();
    setTexture(GU_PSM_8888, 256, 256, pixels);
    sceGuTexFilter(GU_NEAREST, GU_NEAREST);
    const float width = center - r.x, edge = std::min(8.f, width * .5f);
    const float dx[] = {0, edge, width - edge, width},
                sx[] = {0, 8, float(area.w) - 8, float(area.w)};
    for (unsigned part = 0; part < 3; ++part) {
      if (dx[part] >= dx[part + 1])
        continue;
      auto *v = static_cast<TextureVertex *>(
          sceGuGetMemory(2 * sizeof(TextureVertex)));
      float x = ViewX + (r.x + dx[part]) * EditorArt::Scale,
            y = (r.y + (r.h - Packed::SliderHeight) * .5f) * EditorArt::Scale;
      v[0] = {float(area.u) + sx[part], float(area.v), 0xffffffffu, x, y, 0};
      v[1] = {float(area.u) + sx[part + 1],
              float(area.v + area.h),
              0xffffffffu,
              ViewX + (r.x + dx[part + 1]) * EditorArt::Scale,
              y + Packed::SliderHeight * EditorArt::Scale,
              0};
      sceGuDrawArray(GU_SPRITES,
                     GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF |
                         GU_TRANSFORM_2D,
                     2, nullptr, v);
    }
  }
  const auto &knob = Packed::Knobs[lang][state == 3 ? 2 : state ? 1 : 0];
  frame(knob.frame, r.x + float((r.w - knob.w) * fraction),
        r.y + (r.h - knob.h) * .5f);
}
inline bool drawBack(const char *exe, bool ru, double time, float sourceY) {
  if (!load(exe))
    return false;
  if (!drawOpen)
    beginBatch();
  const auto previous = EditorArt::uiScissor;
  EditorArt::scissor(172, 0, 444, 272);
  unsigned lang = unsigned(ru),
           index = unsigned(
                       std::max(0., std::floor(time * Packed::BackFps[lang]))) %
                   59;
  if(ru)frame(Packed::Back[lang][index],264,sourceY);else browserBackScale068(Packed::Back[lang][index],264,sourceY);
  EditorArt::scissor(previous[0], previous[1], previous[2], previous[3]);
  return error.empty();
}
inline bool draw(const char *exe, bool ru, const Values &v, int hover,
                 int pressed, bool dirty, double time) {
  if(!load(exe))return false;
  if(!drawOpen)beginBatch();
  const auto previous=EditorArt::uiScissor;EditorArt::scissor(172,0,444,272);
  const unsigned bg=unsigned(std::max(0.,std::floor(time*Packed::BackgroundFps)))%std::size(Packed::Background);
  frame(Packed::Background[bg]);
  const bool good=EditorSettingsMainUi068::draw(exe,v,hover,pressed,dirty,time);
  EditorArt::scissor(previous[0],previous[1],previous[2],previous[3]);
  (void)ru;return good||fail("Cannot read Main settings artwork");
}
} // namespace EditorSettingsArt
