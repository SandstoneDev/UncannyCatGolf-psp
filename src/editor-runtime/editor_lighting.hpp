#pragma once
// Original Godot 4.4 canvas PointLight2D ADD equation. Full source normal,
// specular and radial-mask bytes are streamed from editor-light.bin.
namespace Lighting {
struct FloatColor {
  float r = 1, g = 1, b = 1, a = 1;
};
inline FloatColor ambient{};
struct Point {
  float x, y, xx, xy, yx, yy, r, g, b, a, extentX, extentY;
};
inline std::vector<Point> points;
inline std::array<unsigned, 16> active{};
inline unsigned count = 0;
inline uint64_t signature = 0;
inline FILE *bank = nullptr;
inline std::array<std::array<uint32_t, 4>, 5> index{};
inline std::array<uint32_t, 128 * 128> mask{};
struct Row {
  unsigned image = UINT_MAX, y = 0, x = 0, age = 0;
  std::array<uint32_t, 256> pixels{};
};
inline std::array<Row, 16> rows{};
inline unsigned age = 0;
inline uint32_t*tileResident=nullptr;
inline constexpr unsigned TilePixels=640*64;
inline unsigned residentBytes(){return tileResident?TilePixels*2*sizeof(uint32_t):0;}
inline bool opened() {
  if (bank)
    return true;
  auto name = companion(executable.c_str(), "data/world/editor-light.bin");
  bank = std::fopen(name.c_str(), "rb");
  uint32_t header[4]{};
  if (!bank || std::fread(header, sizeof header, 1, bank) != 1 ||
      header[0] != 0x4c454355 || header[1] != 1 || header[2] != 5 ||
      std::fread(index.data(), sizeof index, 1, bank) != 1 ||
      index[0][1] != 128 || index[0][2] != 128 ||
      std::fseek(bank, long(index[0][0]), SEEK_SET) ||
      std::fread(mask.data(), sizeof mask, 1, bank) != 1) {
    if (bank)
      std::fclose(bank);
    bank = nullptr;
    fail("Cannot read original light/normal/specular bank: " + name);
    return false;
  }
  // The two small original atlases are sampled for every visible tile pixel.
  // Keep their exact RGBA bytes resident when allocation succeeds; the bounded
  // row cache below remains the lossless allocation-failure path.
  if(index[1][1]!=640||index[1][2]!=64||index[2][1]!=640||index[2][2]!=64){std::fclose(bank);bank=nullptr;fail("Invalid original tile light atlas dimensions");return false;}
  tileResident=static_cast<uint32_t*>(std::malloc(TilePixels*2*sizeof(uint32_t)));
  if(tileResident)for(unsigned i=1;i<=2;++i)if(std::fseek(bank,long(index[i][0]),SEEK_SET)||std::fread(tileResident+(i-1)*TilePixels,sizeof(uint32_t),TilePixels,bank)!=TilePixels){std::free(tileResident);tileResident=nullptr;std::fclose(bank);bank=nullptr;fail("Cannot read original resident tile normal/specular atlas");return false;}
  return true;
}
inline void close() {
  if (bank)
    std::fclose(bank);
  bank = nullptr;
  std::free(tileResident);tileResident=nullptr;
  for (auto &r : rows)
    r.image = UINT_MAX;
  points.clear();
  count = 0;
}
inline uint32_t pixel(unsigned image, int x, int y) {
  if (image >= index.size() || !opened())
    return 0;
  int w = int(index[image][1]), h = int(index[image][2]);
  x = (x % w + w) % w;
  y = (y % h + h) % h;
  if(tileResident&&(image==1||image==2))return tileResident[(image-1)*TilePixels+unsigned(y)*640+unsigned(x)];
  unsigned bx = unsigned(x) & ~255u;
  for (auto &r : rows)
    if (r.image == image && r.y == unsigned(y) && r.x == bx) {
      r.age = ++age;
      return r.pixels[unsigned(x) - bx];
    }
  Row *r = &rows[0];
  for (auto &candidate : rows)
    if (candidate.image == UINT_MAX) {
      r = &candidate;
      break;
    } else if (candidate.age < r->age)
      r = &candidate;
  unsigned n = std::min(256u, unsigned(w) - bx);
  if (std::fseek(bank,
                 long(index[image][0] + (unsigned(y) * unsigned(w) + bx) * 4),
                 SEEK_SET) ||
      std::fread(r->pixels.data(), 4, n, bank) != n) {
    fail("Cannot read original normal/specular scanline");
    return 0;
  }
  r->image = image;
  r->y = unsigned(y);
  r->x = bx;
  r->age = ++age;
  return r->pixels[unsigned(x) - bx];
}
inline void add(float x, float y, float sx, float sy, float angle, Color c) {
  if (sx == 0 || sy == 0 || c.a == 0)
    return;
  float ca = std::cos(angle), sa = std::sin(angle);
  points.push_back({x, y, ca / sx, sa / sx, -sa / sy, ca / sy, float(c.r),
                    float(c.g), float(c.b), float(c.a),
                    64 * (std::abs(ca * sx) + std::abs(sa * sy)),
                    64 * (std::abs(sa * sx) + std::abs(ca * sy))});
}
inline void select(float l, float t, float r, float b) {
  count = 0;
  for (unsigned i = 0; i < points.size() && count < active.size(); ++i) {
    const auto &p = points[i];
    if (p.x + p.extentX >= l && p.x - p.extentX <= r && p.y + p.extentY >= t &&
        p.y - p.extentY <= b)
      active[count++] = i;
  }
}
inline float channel(uint32_t p, unsigned shift) {
  return float((p >> shift) & 255) * (1.f / 255);
}
// alpha() has already clipped finite inputs to [0,128), then subtracts
// 0.5. Truncation is floor on nonnegative values; the remaining negative
// interval has floor -1. Preserve the original fallback outside that domain.
inline int insideFloor(float v) {
  if (v >= -.5f && v < 128.f)
    return v < 0.f ? -1 : int(v);
  return int(std::floor(v));
}
inline float alpha(float x, float y) {
  if (x < 0 || y < 0 || x >= 128 || y >= 128)
    return 0;
  // The source light atlas is linearly filtered, even when sprite textures are
  // nearest.
  x -= .5f;
  y -= .5f;
  int ix = insideFloor(x), iy = insideFloor(y);
  float fx = x - ix, fy = y - iy;
  auto a = [](int px, int py) {
    return channel(mask[unsigned(std::clamp(py, 0, 127)) * 128 +
                        unsigned(std::clamp(px, 0, 127))],
                   24);
  };
  float top = a(ix, iy) * (1 - fx) + a(ix + 1, iy) * fx,
        bot = a(ix, iy + 1) * (1 - fx) + a(ix + 1, iy + 1) * fx;
  return top * (1 - fy) + bot * fy;
}
struct Normal {
  float x = 0, y = 0, z = 1;
  bool used = false;
};
inline Normal normal(uint32_t rgba, float depth = 1, bool shader = false,
                     float ca = 1, float sa = 0, float signX = 1,
                     float signY = 1) {
  float x = channel(rgba, 0) * 2 - 1, y = 1 - channel(rgba, 8) * 2, z;
  if (shader) {
    x *= depth;
    y *= depth;
    z = 1 + depth * (channel(rgba, 16) - 1);
  } else
    z = std::sqrt(std::max(0.f, 1 - x * x - y * y));
  float n = 1 / std::sqrt(std::max(.000001f, x * x + y * y + z * z));
  x *= n * signX;
  y *= n * signY;
  z *= n;
  return {x * ca - y * sa, x * sa + y * ca, z, true};
}
inline uint32_t shade(uint32_t source, float x, float y, FloatColor tint = {},
                      Normal n = {}, uint32_t spec = 0, bool specular = false,
                      float shininess = .14f) {
  float br = channel(source, 0) * tint.r, bg = channel(source, 8) * tint.g,
        bb = channel(source, 16) * tint.b;
  float r = br * ambient.r, g = bg * ambient.g, b = bb * ambient.b;
  for (unsigned j = 0; j < count; ++j) {
    const auto &p = points[active[j]];
    float dx = x - p.x, dy = y - p.y;
    float a =
        alpha(p.xx * dx + p.xy * dy + 64, p.yx * dx + p.yy * dy + 64) * p.a;
    if (a == 0)
      continue;
    float diffuse = 1, s = 0;
    if (n.used) {
      float inv = 1 / std::sqrt(std::max(.000001f, dx * dx + dy * dy));
      float lx = -dx * inv, ly = -dy * inv;
      diffuse = std::max(0.f, n.x * lx + n.y * ly);
      if (specular) {
        constexpr float invSqrt2 = .7071067811865475f;
        float ndh = std::max(0.f, (n.x * lx + n.y * ly + n.z) * invSqrt2);
        float exponent =
            std::exp2(15 * shininess * channel(spec, 24) + 1) * .25f;
        s = std::pow(ndh, exponent) * (exponent + 8) *
            (1.f / (8 * 3.14159265358979323846f)) /
            std::max(4 * std::max(n.z, 0.f) * diffuse, .75f);
      }
    }
    r += (br * diffuse + channel(spec, 0) * s) * p.r * a;
    g += (bg * diffuse + channel(spec, 8) * s) * p.g * a;
    b += (bb * diffuse + channel(spec, 16) * s) * p.b * a;
  }
  auto byte = [](float v) {
    return uint32_t(std::clamp(v * 255 + .5f, 0.f, 255.f));
  };
  return byte(r) | (byte(g) << 8) | (byte(b) << 16) |
         (byte(channel(source, 24) * tint.a * ambient.a) << 24);
}
inline FloatColor tint(Color c) {
  return {float(c.r), float(c.g), float(c.b), float(c.a)};
}
} // namespace Lighting
