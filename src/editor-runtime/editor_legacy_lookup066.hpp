#pragma once
#include "editor_legacy_fonts_data.hpp"
#include <iterator>
namespace EditorLegacyLookup066 {
using Glyph=EditorRender::LegacyFonts::Glyph;
inline constexpr unsigned AsciiFirst=32,AsciiCount=95,CyrillicFirst=1024,CyrillicCount=96;
inline constexpr unsigned DirectCount=AsciiCount+CyrillicCount;
// Frozen original banks have two contiguous codepoint runs followed by a
// small, unsorted punctuation/ligature suffix. Fail compilation if that layout
// changes, rather than silently applying an invalid binary/direct lookup.
inline constexpr bool validPrefix(){
  using namespace EditorRender::LegacyFonts;
  for(const auto&font:Fonts){
    if(font.count<DirectCount||font.first>std::size(Glyphs)||font.count>std::size(Glyphs)-font.first)return false;
    for(unsigned i=0;i<AsciiCount;++i)if(Glyphs[font.first+i].code!=AsciiFirst+i)return false;
    for(unsigned i=0;i<CyrillicCount;++i)if(Glyphs[font.first+AsciiCount+i].code!=CyrillicFirst+i)return false;
  }
  return true;
}
static_assert(validPrefix(),"Legacy font prefix changed; update exact glyph lookup");
inline const Glyph*glyph(unsigned font,unsigned code){
  using namespace EditorRender::LegacyFonts;
  if(font>=std::size(Fonts))return nullptr;
  const auto&f=Fonts[font];
  if(code-AsciiFirst<AsciiCount)return &Glyphs[f.first+code-AsciiFirst];
  if(code-CyrillicFirst<CyrillicCount)return &Glyphs[f.first+AsciiCount+code-CyrillicFirst];
  for(unsigned i=DirectCount;i<f.count;++i){const auto&g=Glyphs[f.first+i];if(g.code==code)return &g;}
  return nullptr;
}
}
