#pragma once
namespace PickerRuntime {
inline uint8_t *pixels = nullptr;
inline bool attempted = false;
inline unsigned reads = 0;
inline void release() {
  flushBatch();
  std::free(pixels);
  pixels = nullptr;
  attempted = false;
}
inline bool load() {
  if (pixels)
    return true;
  if (attempted)
    return false;
  attempted = true;
  const auto file =
      companion(executable.c_str(), "data/world/editor-icons.bin");
  auto *f = std::fopen(file.c_str(), "rb");
  uint32_t header[4]{};
  if (!f || std::fread(header, sizeof header, 1, f) != 1 ||
      header[0] != 0x49454355 || header[1] != 1 || header[2] != PageBytes) {
    if (f)
      std::fclose(f);
    fail("Cannot open original fixed-size picker icons: " + file);
    return false;
  }
  auto *p = static_cast<uint8_t *>(memalign(64, PageBytes));
  bool ok = p && std::fread(p, 1, PageBytes, f) == PageBytes &&
            crc32(0, p, PageBytes) == header[3];
  std::fclose(f);
  if (!ok) {
    std::free(p);
    fail("Cannot allocate/read exact picker icon atlas: " + file);
    return false;
  }
  pixels = p;
  ++reads;
  sceKernelDcacheWritebackRange(pixels, PageBytes);
  markTextureDirty();
  return true;
}
inline bool draw(unsigned image, float x, float y, float w, float h) {
  const PickerIcons::Icon *icon = nullptr;
  for (const auto &i : PickerIcons::Icons)
    if (i.image == image && std::abs(w - i.size) < .01f &&
        std::abs(h - i.size) < .01f) {
      icon = &i;
      break;
    }
  if (!icon)
    return false;
  if (!load())
    return true; // The surfaced I/O error must not trigger the huge-world
                 // decode path.
  const auto previousClip = clip;
  const auto previousScissor = EditorArt::uiScissor;
  clip = {float(std::max(36, previousScissor[0])),
          float(std::max(0, previousScissor[1])),
          float(std::min(444, previousScissor[2])),
          float(std::min(272, previousScissor[3]))};
  std::array<Vertex, 4> q = {
      uiVertex({x, y}, float(icon->u), float(icon->v)),
      uiVertex({x + w, y}, float(icon->u + icon->size), float(icon->v)),
      uiVertex({x + w, y + h}, float(icon->u + icon->size),
               float(icon->v + icon->size)),
      uiVertex({x, y + h}, float(icon->u), float(icon->v + icon->size))};
  polygonTexture(pixels, 3, 256, 256, q.data(), 4, White);
  clip = previousClip;
  EditorArt::scissor(previousScissor[0], previousScissor[1], previousScissor[2],
                     previousScissor[3]);
  return true;
}
} // namespace PickerRuntime
