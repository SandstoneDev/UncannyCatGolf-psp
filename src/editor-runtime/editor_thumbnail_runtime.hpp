#pragma once
namespace ThumbnailRuntime {
struct Thumbnail {
  std::string path;
  float w = 0, h = 0;
  struct Part {
    unsigned offset, x, y, w, h, tw, th;
  };
  unsigned age = 0, width = 0, height = 0, count = 0, sourceSquare=0;
  std::array<Part, 4> parts{};
  uint8_t *pixels = nullptr;
  EditorImages::Info info{};
};
inline std::array<Thumbnail, 4> items{};
inline unsigned reads = 0;
inline void release() {
  flushBatch();
  for (auto &i : items) {
    std::free(i.pixels);
    i = Thumbnail{};
  }
}
inline Thumbnail *prepare(const std::string &file, float w, float h, unsigned sourceSquare=0) {
  for (auto &i : items)
    if (i.pixels && i.path == file && i.w == w && i.h == h && i.sourceSquare==sourceSquare) {
      i.age = ++clock;
      return &i;
    }
  Thumbnail *target = &items[0];
  for (auto &i : items)
    if (!i.pixels) {
      target = &i;
      break;
    } else if (i.age < target->age)
      target = &i;
  EditorImages::Info info{};
  if (!EditorImages::inspect(file.c_str(), info)) {
    fail("Cannot read thumbnail image: " + file);
    return nullptr;
  }
  float scale = std::min(w / info.width, h / info.height);
  unsigned width = unsigned(
               std::max(1, int(std::ceil(info.width * scale * Scale)))),
           height = unsigned(
               std::max(1, int(std::ceil(info.height * scale * Scale))));
  if(sourceSquare)width=height=sourceSquare;
  if (width > 512 || height > 512) {
    fail("Browser thumbnail exceeds visible viewport raster: " + file);
    return nullptr;
  }
  flushBatch();
  std::free(target->pixels);
  target->pixels = nullptr;
  Thumbnail fresh;
  fresh.path = file;
  fresh.w = w;
  fresh.h = h;
  fresh.age = ++clock;
  fresh.width = width;
  fresh.height = height;
  fresh.info = sourceSquare?EditorImages::Info{sourceSquare,sourceSquare,info.jpeg}:info;
  fresh.sourceSquare=sourceSquare;
  unsigned bytes = 0;
  for (unsigned y = 0; y < height; y += 256)
    for (unsigned x = 0; x < width; x += 256) {
      unsigned pw = std::min(256u, width - x), ph = std::min(256u, height - y),
               tw = 16, th = 8;
      while (tw < pw)
        tw *= 2;
      while (th < ph)
        th *= 2;
      fresh.parts[fresh.count++] = {bytes, x, y, pw, ph, tw, th};
      bytes += tw * th * 4;
    }
  auto *pixels = static_cast<uint8_t *>(memalign(64, bytes));
  auto *linear = static_cast<uint32_t *>(std::malloc(width * height * 4));
  if (!pixels || !linear) {
    std::free(pixels);
    std::free(linear);
    fail("Cannot allocate exact browser thumbnail: " + file);
    return nullptr;
  }
  if (!(sourceSquare?EditorImages::resizedBilinear(file.c_str(),linear,width,height,width):EditorImages::thumbnail(file.c_str(), linear, width, height, width))) {
    std::free(pixels);
    std::free(linear);
    fail("Cannot decode source browser thumbnail: " + file);
    return nullptr;
  }
  for (unsigned i = 0; i < fresh.count; ++i) {
    const auto &p = fresh.parts[i];
    std::fill_n(scratch, 256 * 256, 0u);
    for (unsigned y = 0; y < p.h; ++y)
      std::memcpy(scratch + y * 256, linear + (p.y + y) * width + p.x, p.w * 4);
    unsigned output = p.offset;
    for (unsigned y = 0; y < p.th; y += 8)
      for (unsigned x = 0; x < p.tw * 4; x += 16)
        for (unsigned row = 0; row < 8; ++row) {
          std::memcpy(pixels + output,
                      reinterpret_cast<const uint8_t *>(scratch) +
                          (y + row) * 1024 + x,
                      16);
          output += 16;
        }
  }
  std::free(linear);
  sceKernelDcacheWritebackRange(pixels, bytes);
  markTextureDirty();
  fresh.pixels = pixels;
  *target = std::move(fresh);
  ++reads;
  return target;
}
inline bool draw(const std::string &file, float x, float y, float w, float h, unsigned sourceSquare=0,uint32_t tint=White) {
  auto *t = prepare(file, w, h,sourceSquare);
  if (!t)
    return false;
  float scale = std::min(w / t->info.width, h / t->info.height),
        dw = t->info.width * scale, dh = t->info.height * scale;
  x += (w - dw) * .5f;
  y += (h - dh) * .5f;
  const auto previousClip = clip;
  const auto previousScissor = EditorArt::uiScissor;
  clip = {float(std::max(36, previousScissor[0])),
          float(std::max(0, previousScissor[1])),
          float(std::min(444, previousScissor[2])),
          float(std::min(272, previousScissor[3]))};
  for (unsigned i = 0; i < t->count; ++i) {
    const auto &p = t->parts[i];
    float l = x + dw * p.x / t->width, r = x + dw * (p.x + p.w) / t->width,
          top = y + dh * p.y / t->height, b = y + dh * (p.y + p.h) / t->height;
    const std::array<Vertex, 4> q = {uiVertex({l, top}, 0, 0),
                                     uiVertex({r, top}, float(p.w), 0),
                                     uiVertex({r, b}, float(p.w), float(p.h)),
                                     uiVertex({l, b}, 0, float(p.h))};
    polygonTexture(t->pixels + p.offset, 3, p.tw, p.th, q.data(), 4, tint);
  }
  clip = previousClip;
  EditorArt::scissor(previousScissor[0], previousScissor[1], previousScissor[2],
                     previousScissor[3]);
  return true;
}
} // namespace ThumbnailRuntime
