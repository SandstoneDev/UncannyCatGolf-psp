#pragma once
#include "editor_color_picker_hue.hpp"
#include "editor_color_picker_layout.hpp"
#include "editor_color_picker_name.hpp"
#include "editor_color_picker_raster.hpp"
#include "editor_text_flow068.hpp"
// Source Godot4.4.1 ColorPickerButton supplementary skins, full source pixels.
// Include after editor_render.hpp; no ownership of GU frame boundaries.
namespace EditorColorPickerArt {
inline EditorRender::Vertex vertex068(EditorRender::Vec2 p,float u,float v){return EditorRender::uiVertex({drawX068(float(p.x)),drawY068(float(p.y))},u,v);}
inline void rectangle068(float x,float y,float w,float h,uint32_t color){rectangle(36+drawX068(x)*EditorRender::Scale,drawY068(y)*EditorRender::Scale,w*drawScale068()*EditorRender::Scale,h*drawScale068()*EditorRender::Scale,color);}
struct ClipGuard {
  EditorRender::Clip prior = EditorRender::clip;
  std::array<int, 4> ui = EditorArt::uiScissor;
  ClipGuard() {
    EditorRender::clip = {float(std::max(36, ui[0])), float(std::max(0, ui[1])),
                          float(std::min(444, ui[2])),
                          float(std::min(272, ui[3]))};
  }
  ~ClipGuard() {
    EditorRender::clip = prior;
    EditorArt::scissor(ui[0], ui[1], ui[2], ui[3]);
  }
};
inline std::string file(const char *name) {
  return ::companion(
      executable.c_str(),
      (std::string("data/world/editor-aux/picker/") + name + ".png").c_str());
}
inline bool image(const char *name, const Rect &dest,
                  EditorArt::Rect region = {0, 0, 0, 0},
                  uint32_t tint = 0xffffffffu) {
  if (dest.w <= 0 || dest.h <= 0)
    return true;
  const auto path = file(name);
  EditorImages::Info info{};
  if (!EditorRender::inspectImage(path.c_str(), info)) {
    EditorRender::fail("Cannot read original ColorPicker skin: " + path);
    return false;
  }
  if (region.w <= 0 || region.h <= 0)
    region = {0, 0, float(info.width), float(info.height)};
  const unsigned failures = EditorRender::failedImages;
  ClipGuard clip;
  EditorRender::customImage(
      path, info, region,
      [&](EditorRender::Vec2 p) {
        return vertex068(
            {dest.x + (p.x - region.x) * dest.w / region.w,
             dest.y + (p.y - region.y) * dest.h / region.h},
            0, 0);
      },
      tint);
  return failures == EditorRender::failedImages;
}
inline Rect shifted(Rect r, float x, float y) {
  r.x += x;
  r.y += y;
  return r;
}
inline bool panel(float x, float y, float w, float h) {
  const float dx[] = {0, 8, w - 8, w}, dy[] = {0, 8, h - 8, h},
              s[] = {0, 8, 24, 32};
  bool ok = true;
  for (unsigned row = 0; row < 3; ++row)
    for (unsigned col = 0; col < 3; ++col)
      ok = image("PickerPopupPanel",
                 {x + dx[col], y + dy[row], dx[col + 1] - dx[col],
                  dy[row + 1] - dy[row]},
                 {s[col], s[row], s[col + 1] - s[col], s[row + 1] - s[row]}) &&
           ok;
  return ok;
}
inline bool namedImage(const char *name, const Rect &dest,
                       EditorArt::Rect region = {0, 0, 0, 0},
                       uint32_t tint = 0xffffffffu) {
  if (dest.w <= 0 || dest.h <= 0)
    return true;
  const auto path = ::companion(
      executable.c_str(),
      (std::string("data/world/editor-aux/picker-named/") + name + ".png")
          .c_str());
  EditorImages::Info info{};
  if (!EditorRender::inspectImage(path.c_str(), info)) {
    EditorRender::fail("Cannot read named ColorPicker skin: " + path);
    return false;
  }
  if (region.w <= 0 || region.h <= 0)
    region = {0, 0, float(info.width), float(info.height)};
  const auto failures = EditorRender::failedImages;
  ClipGuard clip;
  EditorRender::customImage(
      path, info, region,
      [&](EditorRender::Vec2 p) {
        return vertex068(
            {dest.x + (p.x - region.x) * dest.w / region.w,
             dest.y + (p.y - region.y) * dest.h / region.h},
            0, 0);
      },
      tint);
  return failures == EditorRender::failedImages;
}
inline bool drawPaletteName(const Layout &m, float x, float y,
                            const char *name) {
  if (!name || !*name || m.paletteName.w <= 0)
    return true;
  const auto saved = EditorArt::uiScissor;
  const Rect r = shifted(m.paletteName, x, y);
  const int l = std::max(saved[0],
                         36 + int(std::floor(drawX068(r.x) * EditorRender::Scale))),
            t = std::max(saved[1], int(std::floor(drawY068(r.y) * EditorRender::Scale))),
            right = std::min(
                saved[2],
                36 + int(std::ceil(drawX068(r.x + r.w) * EditorRender::Scale))),
            bottom = std::min(
                saved[3], int(std::ceil(drawY068(r.y + r.h) * EditorRender::Scale)));
  if (l >= right || t >= bottom)
    return true;
  EditorArt::scissor(l, t, right, bottom);
  const float offset068=EditorTextFlow068::offset(NameFont::width(name)*drawScale068(),r.w*drawScale068())/drawScale068();
  bool ok = true;
  NameFont::shape(NameFont::characters(name), [&](const NameFont::Glyph &g,
                                                  float pen) {
    const float whole = std::floor(pen + .125f);
    const unsigned phase = unsigned(std::floor(4 * (pen + .125f)) - 4 * whole);
    const auto &f = g.frames[phase];
    if (!f.w || !f.h)
      return;
    const std::string path =
        ::companion(executable.c_str(), NameFont::Paths[f.page]);
    EditorImages::Info info{};
    if (!EditorRender::inspectImage(path.c_str(), info)) {
      EditorRender::fail("Cannot read original palette-name font: " + path);
      ok = false;
      return;
    }
    ClipGuard guard;
    const auto errors = EditorRender::failedImages;
    EditorRender::customImage(
        path, info, {float(f.u), float(f.v), float(f.w), float(f.h)},
        [&](EditorRender::Vec2 p) {
          return vertex068(
              {std::round(r.x) + whole + f.ox-offset068 + p.x - f.u,
               std::round(r.y) + NameFont::Ascent + f.oy + p.y - f.v},
              0, 0);
        },
        0xffffffffu);
    ok = ok && errors == EditorRender::failedImages;
  });
  EditorArt::scissor(saved[0], saved[1], saved[2], saved[3]);
  return ok;
}
inline bool coreNine(const char *name, EditorArt::Rect source,
                     const Rect &dest) {
  float dx[] = {0, 4, dest.w - 4, dest.w}, dy[] = {0, 4, dest.h - 4, dest.h},
        sx[] = {source.x, source.x + 4, source.x + source.w - 4,
                source.x + source.w},
        sy[] = {source.y, source.y + 4, source.y + source.h - 4,
                source.y + source.h};
  bool ok = true;
  for (unsigned row = 0; row < 3; ++row)
    for (unsigned col = 0; col < 3; ++col)
      ok = namedImage(name,
                      {dest.x + dx[col], dest.y + dy[row],
                       dx[col + 1] - dx[col], dy[row + 1] - dy[row]},
                      {sx[col], sy[row], sx[col + 1] - sx[col],
                       sy[row + 1] - sy[row]}) &&
           ok;
  return ok;
}
inline bool drawBase(const Layout &m, float x, float y, unsigned mode,
                     bool hexVisible, bool swatchesHover, bool recentHover,
                     unsigned addState, const char *name) {
  char blank[64], original[80], header[80];
  mode %= 4;
  std::snprintf(blank, sizeof(blank), "core-blank-%u", mode);
  std::snprintf(original, sizeof(original), "PickerCore_184_%u_1", mode);
  bool ok = panel(x - 4, y - 4, m.w + 8, m.h + 8);
  auto copy = [&](EditorArt::Rect from, Rect to) {
    return namedImage(blank, shifted(to, x, y), from);
  };
  ok = copy({4, 72, 28, 16}, m.sampler) && ok;
  ok = copy({152, 72, 28, 16}, m.shape) && ok;
  const float textWidths[] = {36, 33, 35};
  for (unsigned i = 0; i < 3; ++i) {
    const float sx = 4 + i * 49, tw = textWidths[i],
                caption = std::floor((45 - tw) * .5f),
                left = std::floor((m.mode[i].w - tw) * .5f),
                dx = x + m.mode[i].x, dy = y + 92,
                right = m.mode[i].w - left - tw;
    ok = image(original, {dx, dy, 4, 22}, {sx, 92, 4, 22}) && ok;
    if (left > 4)
      ok = image(original, {dx + 4, dy, left - 4, 22}, {sx + 3, 92, 1, 22}) &&
           ok;
    ok = namedImage(blank,{dx+left,dy,tw,22},{sx+caption,92,tw,22})&&ok;
    constexpr float captionScale068=1.1f;
    ok=image(original,{dx+(m.mode[i].w-tw*captionScale068)*.5f,dy+(22-14*captionScale068)*.5f,tw*captionScale068,14*captionScale068},{sx+caption,96,tw,14})&&ok;
    if (right > 4)
      ok = image(original, {dx + left + tw, dy, right - 4, 22},
                 {sx + 41, 92, 1, 22}) &&
           ok;
    ok = image(original, {dx + m.mode[i].w - 4, dy, 4, 22},
               {sx + 41, 92, 4, 22}) &&
         ok;
  }
  ok = image(original, shifted(m.modeMenu, x, y), {151, 92, 28, 22}) && ok;
  ok = copy({4, 118, 12, 104}, {4, 118, 12, 104}) && ok;
  for (unsigned i = 0; i < 4; ++i)
    ok = copy({122, 118.f + 27 * i, 58, 23}, m.spin[i]) && ok;
  ok = copy({4, 226, 31, 23}, {4, 226, 31, 23}) && ok;
  if (hexVisible) {
    ok = copy({39, 226, 8, 23}, {39, 226, 8, 23}) && ok;
    ok = coreNine(blank, {51, 226, 129, 23}, shifted(m.hex, x, y)) && ok;
  }
  std::snprintf(header, sizeof(header), "PickerHeader_184_0_%u_%u",
                unsigned(m.swatchesOpen), unsigned(swatchesHover));
  ok = image(header,
             {x + m.swatchesHeader.x, y + m.swatchesHeader.y,
              112*ValueScale068, 16*ValueScale068},
             {0, 0, 112, 16}) &&
       ok;
  std::snprintf(header, sizeof(header), "PickerHeader_184_1_%u_%u",
                unsigned(m.recentOpen), unsigned(recentHover));
  ok = image(header,
             {x + m.recentHeader.x, y + m.recentHeader.y,
              112*ValueScale068, 16*ValueScale068},
             {0, 0, 112, 16}) &&
       ok;
  ok = image("PickerIcon_menu_option", shifted(m.paletteMenu, x, y)) && ok;
  if (m.swatchesOpen) {
    std::snprintf(header, sizeof(header),
                  m.sourceButton ? "PickerAdd16_%u" : "PickerAdd_%u",
                  addState % 3);
    ok = image(header, shifted(m.add, x, y)) && ok;
  }
  if(m.presetRows068&&m.swatchesOpen){
    if(m.presetPrevious068.w)ok=image(original,shifted(m.presetPrevious068,x,y),{162,118,18,11})&&ok;
    if(m.presetNext068.w)ok=image(original,shifted(m.presetNext068,x,y),{162,130,18,11})&&ok;
  }
  ok = drawPaletteName(m, x, y, name) && ok;
  return ok;
}
inline bool drawPlane(const Layout &m, float x, float y, unsigned shape,
                      double hueDegrees, double saturation, double value,
                      double okLightness) {
  shape %= 4;
  const auto geometry = shapeLayout(m, shape);
  const auto area = geometry.plane;
  const float planeX = std::round(x + area.x), planeY = std::round(y + area.y);
  const unsigned w = unsigned(area.w), h = unsigned(area.h);
  if (!w || !h || w > 760 || h > 64) {
    EditorRender::fail("Invalid source ColorPicker plane dimensions");
    return false;
  }
  struct Cache {
    bool ready = false;
    float width = 0;
    unsigned shape = 0;
    float hue = 0, s = 0, v = 0, l = 0;
  };
  static Cache cache;
  alignas(64) static uint32_t linear[256 * 64];
  alignas(64) static uint8_t packed[3][256 * 64 * 4];
  alignas(64) static uint8_t overflowPacked[256 * 64 * 4];
  alignas(64) static uint32_t barLinear[16 * 64];
  alignas(64) static uint8_t barPacked[16 * 64 * 4];
  const float hue = float(hueDegrees / 360.), sat = float(saturation),
              val = float(value), light = float(okLightness);
  const auto dimensions = EditorRaster::dimensions(std::min(w, 256u), h);
  const bool changed = !cache.ready || cache.width != area.w ||
                       cache.shape != shape || cache.hue != hue ||
                       cache.s != sat || cache.v != val || cache.l != light;
  if (changed) {
    flushBatch();
    for (unsigned start = 0; start < w; start += 256) {
      const unsigned cw = std::min(256u, w - start);
      for (unsigned yy = 0; yy < h; ++yy)
        for (unsigned xx = 0; xx < cw; ++xx)
          linear[yy * 256 + xx] =
              Raster::pixel(shape, area.w, area.h, start + xx + .5f, yy + .5f,
                            hue, val, light);
      const auto d =
          EditorRaster::pack(linear, 256, packed[start / 256], cw, h);
      sceKernelDcacheWritebackRange(packed[start / 256], d.bytes);
    }
    if (shape == 2 && val > 1) {
      for (unsigned yy = 0; yy < h; ++yy)
        for (unsigned xx = 0; xx < w; ++xx) {
          auto p = Raster::sample(shape, area.w, area.h, xx + .5f, yy + .5f,
                                  hue, val, light);
          linear[yy * 256 + xx] =
              Raster::rgba({std::max(0.f, p.color.r - 1) * p.alpha,
                            std::max(0.f, p.color.g - 1) * p.alpha,
                            std::max(0.f, p.color.b - 1) * p.alpha});
        }
      EditorRaster::pack(linear, 256, overflowPacked, w, h);
      sceKernelDcacheWritebackRange(overflowPacked, dimensions.bytes);
    }
    if (geometry.barVisible) {
      for (unsigned yy = 0; yy < 64; ++yy)
        for (unsigned xx = 0; xx < 15; ++xx)
          barLinear[yy * 16 + xx] =
              shape == 0 ? SourceHue[unsigned((yy + .5f) * 800 / 64)]
                         : Raster::bar(shape, (yy + .5f) / 64, hue, sat);
      EditorRaster::pack(barLinear, 16, barPacked, 15, 64);
      sceKernelDcacheWritebackRange(barPacked, sizeof(barPacked));
    }
    cache = {true, area.w, shape, hue, sat, val, light};
    EditorRender::markTextureDirty();
  }
  ClipGuard guard;
  auto quad = [&](const Rect &r, const uint8_t *p, unsigned tw, unsigned th) {
    std::array<EditorRender::Vertex, 4> q{
        vertex068({std::round(x + r.x), std::round(y + r.y)}, 0,
                               0),
        vertex068({std::round(x + r.x) + r.w, std::round(y + r.y)},
                               r.w, 0),
        vertex068(
            {std::round(x + r.x) + r.w, std::round(y + r.y) + r.h}, r.w, r.h),
        vertex068({std::round(x + r.x), std::round(y + r.y) + r.h},
                               0, r.h)};
    EditorRender::polygonTexture(p, GU_PSM_8888, tw, th, q.data(), 4,
                                 0xffffffffu);
  };
  for (unsigned start = 0; start < w; start += 256) {
    const unsigned cw = std::min(256u, w - start);
    const auto d = EditorRaster::dimensions(cw, h);
    quad({area.x + start, area.y, float(cw), area.h}, packed[start / 256],
         d.width, d.height);
  }
  if (shape == 2 && val > 1) {
    sceGuBlendFunc(GU_ADD, GU_FIX, GU_FIX, 0x00ffffffu, 0x00ffffffu);
    quad(area, overflowPacked, dimensions.width, dimensions.height);
    sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
  }

  if (geometry.barVisible) {
    quad(geometry.bar, barPacked, 16, 64);
    const float fraction =
        shape == 0 ? hue : 1 - std::clamp(shape == 3 ? light : val, 0.f, 1.f);
    auto c = shape == 3 ? Raster::ok(hue, 1, light)
                        : Raster::hsv(hue, 1, shape == 2 ? val : 1);
    const uint32_t inverse = Raster::rgba({1 - c.r, 1 - c.g, 1 - c.b});
    rectangle068(x+geometry.bar.x,y+geometry.bar.y+int(geometry.bar.h*fraction),geometry.bar.w,1,inverse);
  }
  float cx, cy;
  if (shape >= 2) {
    cx = float(int(area.w * .5f +
                   area.w * .5f * std::cos(hue * Raster::Tau) * sat - 6));
    cy = float(int(area.h * .5f +
                   area.h * .5f * std::sin(hue * Raster::Tau) * sat - 6));
  } else {
    const Rect square = shape ? geometry.square : area;
    cx = float(
        int(square.x - area.x + std::clamp(sat, 0.f, 1.f) * square.w - 6));
    cy = float(int(square.y - area.y +
                   (1 - std::clamp(val, 0.f, 1.f)) * square.h - 6));
  }
  const auto current =
      shape == 3 ? Raster::ok(hue, sat, light) : Raster::hsv(hue, sat, val);
  const Rect cursor{planeX + cx, planeY + cy, 12, 12};
  bool ok = image("PickerIcon_picker_cursor_bg", cursor, {0, 0, 12, 12},
                  Raster::rgba(current));
  ok = image("PickerIcon_picker_cursor", cursor) && ok;
  if (shape == 1)
    ok = image("PickerIcon_picker_cursor",
               {planeX + area.w * .5f - 6 +
                    area.w * .5f * std::cos(hue * Raster::Tau) * .92f,
                planeY + area.h * .5f - 6 +
                    area.h * .5f * std::sin(hue * Raster::Tau) * .92f,
                12, 12}) &&
         ok;
  return ok;
}
inline bool drawShape(const Layout &m, float x, float y, unsigned shape,
                      bool hover) {
  char name[80];
  std::snprintf(name, sizeof(name), "PickerShape_%u_%u_%u", 184u, shape % 4,
                unsigned(hover));
  return image(name, shifted(m.shape, x, y));
}
inline bool drawKnob(const Rect &r, float x, float y, double fraction) {
  return image("PickerIcon_bar_arrow",
               {x + r.x + float(int(std::clamp(fraction, 0., 1.) * r.w - 8)),
                y + r.y + 8, 16, 16});
}
inline bool drawChecker(const Rect &r, float x, float y) {
  bool ok = true;
  for (float oy = 0; oy < r.h; oy += 16)
    for (float ox = 0; ox < r.w; ox += 16) {
      float w = std::min(16.f, r.w - ox), h = std::min(16.f, r.h - oy);
      ok = image("PickerIcon_sample_bg", {x + r.x + ox, y + r.y + oy, w, h},
                 {0, 0, w, h}) &&
           ok;
    }
  return ok;
}
inline bool drawRevert(const Rect &r, float x, float y, uint32_t tint) {
  return image(
      "PickerIcon_sample_revert",
      {x + r.x + r.w * .25f - 8, y + r.y + r.h * .95f * .5f - 8, 16, 16},
      {0, 0, 16, 16}, tint);
}
inline void drawText(const Rect&r,float x,float y,const char*value,bool right){
 if(!value||r.w<=8||r.h<=0)return;
 const auto old=EditorArt::uiScissor;const float scale=drawScale068()*ValueScale068;
 const float left=drawX068(x+r.x+TextMargin),available=(r.w-2*TextMargin)*drawScale068();
 const float textWidth=EditorArt::textWidth(0,value)*scale;
 const float tx=left+(right&&textWidth<=available?available-textWidth:0);
 const int l=std::max(old[0],36+int(std::ceil(drawX068(x+r.x+4)*EditorRender::Scale))),t=std::max(old[1],int(std::ceil(drawY068(y+r.y)*EditorRender::Scale))),rr=std::min(old[2],36+int(std::floor(drawX068(x+r.x+r.w-4)*EditorRender::Scale))),b=std::min(old[3],int(std::floor(drawY068(y+r.y+r.h)*EditorRender::Scale)));
 if(l<rr&&t<b){EditorArt::scissor(l,t,rr,b);EditorArt::textScaled(0,value,tx,drawY068(y+r.y+16),TextColor,available-(tx-left),scale);EditorArt::scissor(old[0],old[1],old[2],old[3]);}
}
struct SwatchMasks {
  unsigned w = 0, h = 0;
  std::array<uint32_t, 32 * 32> fill{}, border{};
};
inline std::array<SwatchMasks, 4> swatchMasks{};
inline std::array<uint32_t, 16 * 16> presetChecker{};
inline bool presetCheckerReady = false;
inline SwatchMasks *masks(unsigned w, unsigned h) {
  if ((w != 16 && w != 25) || (h != 16 && h != 25))
    return nullptr;
  auto &m = swatchMasks[(w == 25 ? 2 : 0) + (h == 25 ? 1 : 0)];
  if (m.w)
    return &m;
  char name[80];
  std::snprintf(name, sizeof(name), "PickerSwatch_%u_%u_0", w, h);
  if (!EditorImages::read(file(name).c_str(), {0, 0, w, h}, m.fill.data(),
                          32)) {
    EditorRender::fail("Cannot decode ColorPicker swatch fill");
    return nullptr;
  }
  std::snprintf(name, sizeof(name), "PickerSwatch_%u_%u_2", w, h);
  if (!EditorImages::read(file(name).c_str(), {0, 0, w, h}, m.border.data(),
                          32)) {
    EditorRender::fail("Cannot decode ColorPicker swatch border");
    return nullptr;
  }
  m.w = w;
  m.h = h;
  return &m;
}
inline bool drawSwatch(const Rect &r, float x, float y, double red,
                       double green, double blue, double alpha, bool selected) {
  const unsigned w = unsigned(r.w), h = unsigned(r.h);
  auto *m = masks(w, h);
  if (!m) {
    EditorRender::fail("Invalid original ColorPicker swatch dimensions");
    return false;
  }
  if (!presetCheckerReady) {
    if (!EditorImages::read(file("PickerPresetBg").c_str(), {0, 0, 16, 16},
                            presetChecker.data(), 16)) {
      EditorRender::fail("Cannot decode original ColorPicker checker");
      return false;
    }
    presetCheckerReady = true;
  }
  alignas(64) static uint32_t linear[32 * 32];
  alignas(64) static uint8_t packed[32 * 32 * 4];
  auto byte = [](float v) {
    return uint32_t(std::clamp(std::lround(v * 255.f), 0l, 255l));
  };
  const float rgb[] = {std::clamp(float(red), 0.f, 1.f),
                       std::clamp(float(green), 0.f, 1.f),
                       std::clamp(float(blue), 0.f, 1.f)},
              a = std::clamp(float(alpha), 0.f, 1.f),
              border = selected ? 1.f : 0.f;
  for (unsigned yy = 0; yy < h; ++yy)
    for (unsigned xx = 0; xx < w; ++xx) {
      const unsigned at = yy * 32 + xx;
      float f = float(m->fill[at] >> 24) / 255.f,
            b = float(m->border[at] >> 24) / 255.f, c = f + b, da = 0, d[3]{};
      auto style = [&](const float *color, float opacity) {
        if (c <= 0)
          return;
        float sa = std::clamp(f * opacity + b, 0.f, 1.f);
        for (unsigned k = 0; k < 3; ++k) {
          float sc = (f * color[k] + b * border) / c;
          d[k] = float(byte(sc * sa + d[k] * (1 - sa))) / 255.f;
        }
        da = float(byte(sa + da * (1 - sa))) / 255.f;
      };
      if (a < 1) {
        const float white[] = {1, 1, 1};
        style(white, 1);
        if (xx >= 2 && yy >= 2 && xx < w - 2 && yy < h - 2) {
          const uint32_t checker =
              presetChecker[((yy - 2) % 16) * 16 + (xx - 2) % 16];
          for (unsigned k = 0; k < 3; ++k)
            d[k] = float((checker >> (8 * k)) & 255) / 255.f;
          da = float(checker >> 24) / 255.f;
        }
      }
      style(rgb, a);
      uint32_t pixel = byte(da) << 24;
      for (unsigned k = 0; k < 3; ++k)
        pixel |= byte(da > 0 ? d[k] / da : 0) << (8 * k);
      linear[at] = pixel;
    }
  flushBatch();
  EditorRaster::pack(linear, 32, packed, w, h);
  const auto dimensions = EditorRaster::dimensions(w, h);
  sceKernelDcacheWritebackRange(packed, dimensions.bytes);
  EditorRender::markTextureDirty();
  ClipGuard guard;
  const auto tl = vertex068(
                 {std::round(x + r.x), std::round(y + r.y)}, 0, 0),
             tr = vertex068(
                 {std::round(x + r.x) + r.w, std::round(y + r.y)}, float(w), 0),
             br = vertex068(
                 {std::round(x + r.x) + r.w, std::round(y + r.y) + r.h},
                 float(w), float(h)),
             bl = vertex068(
                 {std::round(x + r.x), std::round(y + r.y) + r.h}, 0, float(h));
  std::array<EditorRender::Vertex, 4> quad{tl, tr, br, bl};
  EditorRender::polygonTexture(packed, GU_PSM_8888, dimensions.width,
                               dimensions.height, quad.data(), 4, 0xffffffffu);
  flushBatch();
  if (red > 1 || green > 1 || blue > 1)
    return image("PickerIcon_overbright_indicator",
                 {std::round(x + r.x), std::round(y + r.y), 16, 16});
  return true;
}
} // namespace EditorColorPickerArt
