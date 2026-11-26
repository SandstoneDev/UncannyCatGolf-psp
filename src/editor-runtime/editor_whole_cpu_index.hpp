#pragma once
#include "editor_whole_cpu_data.hpp"
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <utility>
#include <zlib.h>

// CPU-only exact source pixels. Indices/palettes are never handed to the GE.
namespace EditorWholeCpu {
enum class Asset : uint32_t { Canny = 0, Uncanny = 1, Unknown = UINT32_MAX };
inline constexpr unsigned ChunkBytes = 65536, WorkspaceBytes = 65536,
                          MaxChunks = 9;
inline bool known(Asset id) { return uint32_t(id) < 2; }
// Explicit HUD original IDs only. Filenames never select immutable CPU assets.
inline Asset hudSpriteAsset(unsigned sprite) {
  return sprite == 0   ? Asset::Canny
         : sprite == 1 ? Asset::Uncanny
                       : Asset::Unknown;
}
inline uint32_t little32(const uint8_t *p) {
  return uint32_t(p[0]) | (uint32_t(p[1]) << 8) | (uint32_t(p[2]) << 16) |
         (uint32_t(p[3]) << 24);
}
using ReadAt = bool (*)(void *, uint32_t, void *, size_t);
struct Hooks {
  // Required admission hook: charge Cache::accountedBytes() to the same8MiB
  // renderer budget, then reserve additional bytes (largest<=64KiB).
  bool (*admit)(void *, size_t, size_t) = nullptr;
  void *(*allocate)(void *, size_t) = nullptr;
  void (*release)(void *, void *) = nullptr;
  void *context = nullptr;
};
struct Record {
  uint32_t id = 0, width = 0, height = 0, indexWidth = 0, colors = 0,
           indexBytes = 0, paletteBytes = 0, payloadBytes = 0, offset = 0,
           encodedBytes = 0, payloadCrc = 0, rgbaCrc = 0, encodedCrc = 0;
};
class Reader {
  ReadAt getter = nullptr;
  void *context = nullptr;
  uint32_t length = 0;
  std::array<Record, 2> records{};
  uint64_t generation = 0;

public:
  // Borrowed IO must live through acquire. Leases borrow neither Reader nor IO.
  bool open(ReadAt read, void *ctx, uint32_t bytes, const char *&error) {
    error = nullptr;
    uint8_t header[120];
    if (!read || bytes < sizeof header ||
        !read(ctx, 0, header, sizeof header)) {
      error = "CPU image bank header read failed";
      return false;
    }
    if (little32(header) != 0x49454355 || little32(header + 4) != 1 ||
        little32(header + 8) != 2 || little32(header + 12) != 52) {
      error = "CPU image bank header is incompatible";
      return false;
    }
    std::array<Record, 2> candidate{};
    uint64_t expectedOffset = sizeof header;
    for (unsigned i = 0; i < 2; ++i) {
      uint32_t f[13];
      for (unsigned j = 0; j < 13; ++j)
        f[j] = little32(header + 16 + i * 52 + j * 4);
      Record r{f[0], f[1], f[2], f[3],  f[4],  f[5], f[6],
               f[7], f[8], f[9], f[10], f[11], f[12]};
      const auto &s = Specs[i];
      if (r.id != s.id || r.width != s.width || r.height != s.height ||
          r.indexWidth != s.indexWidth || r.colors != s.colors ||
          r.indexBytes != s.indexBytes || r.paletteBytes != s.paletteBytes ||
          r.payloadBytes != s.payloadBytes || r.payloadCrc != s.payloadCrc ||
          r.rgbaCrc != s.rgbaCrc || r.offset != expectedOffset ||
          !r.encodedBytes || r.encodedBytes >= r.payloadBytes ||
          uint64_t(r.offset) + r.encodedBytes > bytes) {
        error = "CPU image bank record does not match immutable source";
        return false;
      }
      expectedOffset = uint64_t(r.offset) + r.encodedBytes;
      candidate[i] = r;
    }
    if (expectedOffset != bytes) {
      error = "CPU image bank length is invalid";
      return false;
    }
    getter = read;
    context = ctx;
    length = bytes;
    records = candidate;
    if (!++generation)
      ++generation;
    return true;
  }
  void close() {
    getter = nullptr;
    context = nullptr;
    length = 0;
    if (!++generation)
      ++generation;
  }
  bool ready() const { return getter != nullptr; }
  uint64_t epoch() const { return generation; }
  const Record *record(Asset id) const {
    return ready() && known(id) ? &records[uint32_t(id)] : nullptr;
  }
  bool read(uint32_t offset, void *out, size_t bytes) const {
    return getter && offset <= length && bytes <= length - offset &&
           getter(context, offset, out, bytes);
  }
};
struct Stats {
  uint64_t hits = 0, loads = 0, failures = 0, cancelled = 0;
  size_t peakPayloadBytes = 0;
};
class Cache {
  struct Image {
    std::array<uint8_t *, MaxChunks> chunks{};
    std::array<unsigned, MaxChunks> sizes{};
    Record record{};
    const Reader *reader = nullptr;
    uint64_t readerEpoch = 0;
    Hooks owner{};
    unsigned pins = 0;
    bool valid = false;
  };
  std::array<Image, 2> images{};
  size_t liveBytes = 0, pendingBytes = 0;
  uint64_t generation = 1;
  unsigned pins = 0;
  bool loading = false, deferred = false;
  Stats counters{};
  static uint8_t byte(const Image &i, unsigned at) {
    return i.chunks[at >> 16][at & 65535];
  }
  static uint32_t word(const Image &i, unsigned at) {
    return uint32_t(byte(i, at)) | (uint32_t(byte(i, at + 1)) << 8) |
           (uint32_t(byte(i, at + 2)) << 16) |
           (uint32_t(byte(i, at + 3)) << 24);
  }
  static unsigned index(const Image &i, unsigned pixel) {
    const unsigned at = pixel * i.record.indexWidth;
    return i.record.indexWidth == 1
               ? byte(i, at)
               : unsigned(byte(i, at)) | (unsigned(byte(i, at + 1)) << 8);
  }
  static uint32_t pixel(const Image &i, unsigned x, unsigned y) {
    if (!i.valid || x >= i.record.width || y >= i.record.height)
      return 0;
    const unsigned n = index(i, y * i.record.width + x);
    return n < i.record.colors ? word(i, i.record.indexBytes + n * 4) : 0;
  }
  static void dispose(Image &i) {
    if (i.owner.release)
      for (auto *p : i.chunks)
        if (p)
          i.owner.release(i.owner.context, p);
    i = Image{};
  }
  void finishClear() {
    if (loading || pins)
      return;
    for (auto &i : images)
      dispose(i);
    liveBytes = 0;
    deferred = false;
  }
  void unpin(Image &i) {
    --i.pins;
    --pins;
    if (deferred)
      finishClear();
  }
  struct Arena {
    uint8_t *memory;
    unsigned size, used = 0;
  };
  static voidpf zAllocate(voidpf context, uInt count, uInt size) {
    auto &a = *static_cast<Arena *>(context);
    if (!count || !size || count > a.size / size)
      return nullptr;
    const unsigned bytes = count * size;
    const uintptr_t base = reinterpret_cast<uintptr_t>(a.memory);
    const unsigned at =
        unsigned(((base + a.used + 63) & ~uintptr_t(63)) - base);
    if (at > a.size || bytes > a.size - at)
      return nullptr;
    void *p = a.memory + at;
    std::memset(p, 0, bytes);
    a.used = at + bytes;
    return p;
  }
  static void zRelease(voidpf, voidpf) {}
  static bool decode(Image &i, Reader &reader, void *workspace,
                     const char *&error) {
    Arena arena{static_cast<uint8_t *>(workspace), WorkspaceBytes};
    z_stream z{};
    z.zalloc = zAllocate;
    z.zfree = zRelease;
    z.opaque = &arena;
    if (inflateInit2(&z, 13) != Z_OK) {
      error = "CPU image inflate workspace failed";
      return false;
    }
    uint8_t input[4096], overflow = 0;
    unsigned supplied = 0;
    uint32_t compressedCrc = 0;
    bool success = false;
    for (;;) {
      if (!z.avail_in && supplied < i.record.encodedBytes) {
        unsigned count = std::min<unsigned>(
            unsigned(sizeof input), unsigned(i.record.encodedBytes) - supplied);
        if (!reader.read(i.record.offset + supplied, input, count)) {
          error = "CPU image compressed data read failed";
          break;
        }
        compressedCrc = uint32_t(crc32(compressedCrc, input, count));
        supplied += count;
        z.next_in = input;
        z.avail_in = count;
      }
      const unsigned produced = unsigned(z.total_out);
      if (produced < i.record.payloadBytes) {
        unsigned block = produced >> 16, offset = produced & 65535;
        z.next_out = i.chunks[block] + offset;
        z.avail_out = i.sizes[block] - offset;
      } else {
        z.next_out = &overflow;
        z.avail_out = 1;
      }
      const uLong previousIn = z.total_in, previousOut = z.total_out;
      int result = inflate(&z, Z_NO_FLUSH);
      if (z.total_out > i.record.payloadBytes) {
        error = "CPU image inflate exceeds declared size";
        break;
      }
      if (result == Z_STREAM_END) {
        success = z.total_out == i.record.payloadBytes &&
                  z.total_in == i.record.encodedBytes &&
                  supplied == i.record.encodedBytes && !z.avail_in &&
                  compressedCrc == i.record.encodedCrc;
        if (!success)
          error = "CPU image compressed size or checksum differs";
        break;
      }
      if (result != Z_OK ||
          (previousIn == z.total_in && previousOut == z.total_out)) {
        error = "CPU image compressed stream is invalid";
        break;
      }
    }
    inflateEnd(&z);
    if (!success)
      return false;
    uint32_t payloadCrc = 0;
    for (unsigned n = 0; n < MaxChunks; ++n)
      if (i.chunks[n])
        payloadCrc = uint32_t(crc32(payloadCrc, i.chunks[n], i.sizes[n]));
    if (payloadCrc != i.record.payloadCrc) {
      error = "CPU image payload checksum differs";
      return false;
    }
    uint8_t row[512 * 4];
    uint32_t rgbaCrc = 0;
    for (unsigned y = 0; y < i.record.height; ++y) {
      for (unsigned x = 0; x < i.record.width; ++x) {
        const unsigned n = index(i, y * i.record.width + x);
        if (n >= i.record.colors) {
          error = "CPU image palette index is invalid";
          return false;
        }
        const uint32_t c = word(i, i.record.indexBytes + n * 4);
        row[x * 4] = uint8_t(c);
        row[x * 4 + 1] = uint8_t(c >> 8);
        row[x * 4 + 2] = uint8_t(c >> 16);
        row[x * 4 + 3] = uint8_t(c >> 24);
      }
      rgbaCrc = uint32_t(crc32(rgbaCrc, row, i.record.width * 4));
    }
    if (rgbaCrc != i.record.rgbaCrc) {
      error = "CPU image original RGBA checksum differs";
      return false;
    }
    return true;
  }

public:
  class Lease {
    Cache *cache = nullptr;
    Image *image = nullptr;
    Lease(Cache *c, Image *i) : cache(c), image(i) {
      ++c->pins;
      ++i->pins;
    }
    friend class Cache;

  public:
    Lease() = default;
    Lease(const Lease &) = delete;
    Lease &operator=(const Lease &) = delete;
    Lease(Lease &&v) noexcept : cache(v.cache), image(v.image) {
      v.cache = nullptr;
      v.image = nullptr;
    }
    Lease &operator=(Lease &&v) noexcept {
      if (this != &v) {
        reset();
        cache = v.cache;
        image = v.image;
        v.cache = nullptr;
        v.image = nullptr;
      }
      return *this;
    }
    ~Lease() { reset(); }
    void reset() {
      if (cache)
        cache->unpin(*image);
      cache = nullptr;
      image = nullptr;
    }
    explicit operator bool() const { return image != nullptr; }
    uint32_t pixel(unsigned x, unsigned y) const {
      return image ? Cache::pixel(*image, x, y) : 0;
    }
    // Match old256-square zero padding; never clamp to a whole-image edge.
    uint32_t chunkPixel(unsigned sx, unsigned sy, unsigned sw, unsigned sh,
                        uint32_t at) const {
      if (!image || at == UINT32_MAX || at >= 65536)
        return 0;
      unsigned u = at & 255, v = at >> 8;
      if (u >= sw || v >= sh || sx > image->record.width ||
          sy > image->record.height || u >= image->record.width - sx ||
          v >= image->record.height - sy)
        return 0;
      return pixel(sx + u, sy + v);
    }
    unsigned width() const { return image ? image->record.width : 0; }
    unsigned height() const { return image ? image->record.height : 0; }
    // Expand only an original source chunk for the ordinary GE texture path.
    // Output remains the exact padded/swizzled256-square RGBA page; projection,
    // filtering and the shader are unchanged. Never expose CPU index storage.
    bool swizzleRegion(unsigned sx,unsigned sy,unsigned w,unsigned h,void*target,size_t targetBytes)const{
      if(!image||!target||targetBytes<262144||!w||!h||w>256||h>256||
         sx>image->record.width||sy>image->record.height||w>image->record.width-sx||h>image->record.height-sy)return false;
      const uintptr_t destination=reinterpret_cast<uintptr_t>(target);
      for(unsigned i=0;i<MaxChunks;++i)if(image->chunks[i]){
        const uintptr_t source=reinterpret_cast<uintptr_t>(image->chunks[i]);
        if(destination<=source?source-destination<262144:destination-source<image->sizes[i])return false;
      }
      auto*out=static_cast<uint8_t*>(target);std::memset(out,0,262144);
      for(unsigned y=0;y<h;++y){
        const unsigned first=((sy+y)*image->record.width+sx)*image->record.indexWidth;
        for(unsigned x=0;x<w;++x){
          const unsigned at=first+x*image->record.indexWidth;
          const auto*indices=image->chunks[at>>16]+(at&65535);
          const unsigned index=image->record.indexWidth==1?*indices:unsigned(indices[0])|(unsigned(indices[1])<<8);
          if(index>=image->record.colors)return false;
          const unsigned paletteAt=image->record.indexBytes+index*4;
          const auto*color=image->chunks[paletteAt>>16]+(paletteAt&65535);
          const unsigned offset=((y/8)*64+x/4)*128+(y%8)*16+(x%4)*4;
          std::memcpy(out+offset,color,4);
        }
      }
      return true;
    }
  };
  Cache() = default;
  Cache(const Cache &) = delete;
  Cache &operator=(const Cache &) = delete;
  ~Cache() { clear(); } // Global renderer owner must outlive all Leases.
  size_t payloadBytes() const { return liveBytes + pendingBytes; }
  size_t accountedBytes() const { return sizeof(Cache) + payloadBytes(); }
  static constexpr size_t fixedBytes() { return sizeof(Cache); }
  const Stats &stats() const { return counters; }
  bool clear() {
    if (!++generation)
      ++generation;
    deferred = true;
    finishClear();
    return !deferred;
  }
  bool clearPending() const { return deferred; }
  Lease acquire(Asset id, Reader &reader, const Hooks &hooks, void *workspace,
                size_t workspaceBytes, const char *&error) {
    error = nullptr;
    auto fail = [&](const char *message) {
      error = message;
      ++counters.failures;
      return Lease{};
    };
    const auto *record = reader.record(id);
    if (!record)
      return fail("CPU image asset is unknown or bank unavailable");
    if (loading || deferred)
      return fail("CPU image cache is busy or awaiting reclaim");
    auto &current = images[uint32_t(id)];
    if (current.valid && current.reader == &reader &&
        current.readerEpoch == reader.epoch()) {
      ++counters.hits;
      return Lease(this, &current);
    }
    if (current.pins)
      return fail("CPU image source replacement is currently borrowed");
    if (!workspace || workspaceBytes < WorkspaceBytes || !hooks.admit ||
        !hooks.allocate || !hooks.release)
      return fail("CPU image requires bounded workspace and memory hooks");
    Image pending;
    pending.record = *record;
    pending.reader = &reader;
    pending.readerEpoch = reader.epoch();
    pending.owner = hooks;
    const uint64_t started = generation;
    loading = true;
    const bool admitted =
        hooks.admit(hooks.context, record->payloadBytes,
                    std::min(record->payloadBytes, uint32_t(ChunkBytes)));
    bool success = admitted;
    if (!admitted)
      error = "CPU image memory budget denied";
    if (success && started != generation) {
      success = false;
      error = "CPU image acquisition was cancelled";
    }
    for (unsigned n = 0;
         success && n < MaxChunks && pendingBytes < record->payloadBytes; ++n) {
      const unsigned bytes = std::min<unsigned>(
          unsigned(ChunkBytes),
          unsigned(pending.record.payloadBytes) - unsigned(pendingBytes));
      auto *p = static_cast<uint8_t *>(hooks.allocate(hooks.context, bytes));
      if (!p) {
        success = false;
        error = "CPU image chunk allocation failed";
        break;
      }
      pending.chunks[n] = p;
      pending.sizes[n] = bytes;
      pendingBytes += bytes;
      counters.peakPayloadBytes =
          std::max(counters.peakPayloadBytes, payloadBytes());
      if (started != generation) {
        success = false;
        error = "CPU image acquisition was cancelled";
      }
    }
    if (success)
      success = decode(pending, reader, workspace, error);
    if (success &&
        (started != generation || pending.readerEpoch != reader.epoch())) {
      success = false;
      error = "CPU image acquisition epoch changed";
    }
    pendingBytes = 0;
    if (success) {
      const size_t old = current.valid ? current.record.payloadBytes : 0;
      dispose(current);
      pending.valid = true;
      current = pending;
      liveBytes = liveBytes - old + current.record.payloadBytes;
      ++counters.loads;
    } else {
      dispose(pending);
      ++counters.failures;
      if (started != generation)
        ++counters.cancelled;
    }
    loading = false;
    if (deferred) {
      if (success) {
        success = false;
        error = "CPU image publication was reclaimed";
        ++counters.cancelled;
      }
      finishClear();
    }
    return success ? Lease(this, &current) : Lease{};
  }
};
} // namespace EditorWholeCpu
