#pragma once
#include "editor_color_picker_name_data.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <iterator>
#include <vector>
namespace EditorColorPickerArt {
namespace NameFont {
inline const Glyph *glyph(unsigned code) {
  auto first = std::begin(Glyphs), last = std::end(Glyphs);
  auto at = std::lower_bound(
      first, last, code, [](const Glyph &g, unsigned c) { return g.code < c; });
  return at != last && at->code == code ? at : nullptr;
}
inline unsigned codepoint(const char *&s) {
  unsigned c = static_cast<unsigned char>(*s++);
  if (c < 128)
    return c;
  unsigned n = c < 224 ? 1 : c < 240 ? 2 : 3;
  c &= n == 1 ? 31 : n == 2 ? 15 : 7;
  while (n-- && *s)
    c = (c << 6) | (static_cast<unsigned char>(*s++) & 63);
  return c;
}
inline std::vector<unsigned> characters(const char *text) {
  std::vector<unsigned> out;
  if (!text)
    return out;
  while (*text) {
    unsigned code = 0;
    size_t bytes = 0;
    for (const auto &l : Ligatures) {
      const size_t n = std::strlen(l.text);
      if (n > bytes && std::strncmp(text, l.text, n) == 0) {
        code = l.code;
        bytes = n;
      }
    }
    if (bytes)
      text += bytes;
    else
      code = codepoint(text);
    out.push_back(glyph(code) ? code : 63);
  }
  return out;
}
inline const Kern *pair(unsigned left, unsigned right) {
  for (const auto &k : Kerning)
    if (k.left == left && k.right == right)
      return &k;
  return nullptr;
}
template <class Visitor>
inline float shape(const std::vector<unsigned> &text, const Visitor &visit) {
  float pen = 0;
  for (unsigned i = 0; i < text.size(); ++i) {
    const auto *g = glyph(text[i]);
    if (!g)
      continue;
    const auto *before = i ? pair(text[i - 1], text[i]) : nullptr;
    const auto *after =
        i + 1 < text.size() ? pair(text[i], text[i + 1]) : nullptr;
    const float advance = g->advance + (before ? before->db : 0) +
                          (after ? after->da : 0),
                offset = (before ? before->ob : 0) + (after ? after->oa : 0);
    visit(*g, pen + offset);
    pen += advance;
  }
  return pen;
}
inline float width(const char *text) {
  return std::ceil(shape(characters(text), [](const Glyph &, float) {}));
}
} // namespace NameFont
} // namespace EditorColorPickerArt
