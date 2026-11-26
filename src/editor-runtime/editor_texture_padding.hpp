#pragma once
#include <cstdint>
#include <cstring>
namespace EditorTexturePadding {
// EditorImages::read writes only the requested source rectangle. The remaining
// 256-square upload area must not inherit colors from an earlier decode.
inline bool clear(uint32_t *page, unsigned width, unsigned height) {
  if (!page || width > 256 || height > 256)
    return false;
  if (width < 256)
    for (unsigned y = 0; y < height; ++y)
      std::memset(page + y * 256 + width, 0, (256 - width) * sizeof(*page));
  if (height < 256)
    std::memset(page + height * 256, 0,
                (256 - height) * 256 * sizeof(*page));
  return true;
}
} // namespace EditorTexturePadding
