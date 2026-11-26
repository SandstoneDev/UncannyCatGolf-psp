#pragma once
// Included inside EditorRender; caller supplies zlib and standard C++ headers.
namespace Cold {
inline constexpr unsigned WorkspaceBytes = 65536, ChunkBytes = 4096;
enum class Result {
  Stored,
  AlreadyPresent,
  NotUseful,
  AllocationFailed,
  BudgetExceeded,
  Invalid
};
struct Key {
  unsigned id = UINT_MAX;
  const char *file = nullptr;
  unsigned x = 0, y = 0, w = 0, h = 0;
};
struct Block {
  Block *next = nullptr;
  unsigned used = 0;
  uint8_t data[ChunkBytes];
};
struct Entry {
  unsigned id = UINT_MAX;
  char *file = nullptr;
  unsigned x = 0, y = 0, w = 0, h = 0;
  unsigned format = 0, colors = 0, rawBytes = 0, crc = 0, age = 0,
           encodedBytes = 0, allocatedBytes = 0, serial = 0;
  Block *blocks = nullptr;
};
struct SegmentPool {
  static constexpr unsigned SegmentBytes = 65536, SlotBytes = 4112,
                            SlotsPerSegment = SegmentBytes / SlotBytes,
                            MaxSegments = 32,
                            MaxSlots = SlotsPerSegment * MaxSegments;
  using Allocate = void *(*)(size_t);
  using Release = void (*)(void *);
  using Evict = bool (*)();
  std::array<uint8_t *, MaxSegments> segments{};
  void *freeHead = nullptr;
  void *owner = nullptr;
  Release segmentRelease = nullptr;
  Evict evict = nullptr;
  unsigned segmentCount = 0, usedSlots = 0, highSlots = 0, requests = 0,
           failures = 0, evictions = 0, invalidReleases = 0;
  const char *error = "";
  std::array<uint8_t, (MaxSlots + 7) / 8> busy{};
  unsigned residentBytes() const { return segmentCount * SegmentBytes; }
  bool ready() const { return segmentCount != 0; }
  // Called only by the render thread after its existing completed-frame fence.
  // Live cells never move. Rebuild indices/free links when empty segments go.
  // Keep one segment so existing pool allocator callbacks remain usable.
  unsigned releaseUnusedSegments() {
    if (segmentCount < 2 || !segmentRelease)
      return 0;
    const auto oldBusy = busy;
    const unsigned oldCount = segmentCount;
    unsigned observed = 0;
    for (unsigned i = 0; i < oldCount * SlotsPerSegment; ++i)
      observed += bool(oldBusy[i / 8] & (1u << (i % 8)));
    if (observed != usedSlots) {
      error = "Cold pool live-cell accounting mismatch";
      return 0;
    }
    busy.fill(0);
    unsigned kept = 0, freed = 0;
    for (unsigned segment = 0; segment < oldCount; ++segment) {
      bool live = false;
      for (unsigned slot = 0; slot < SlotsPerSegment; ++slot) {
        const unsigned at = segment * SlotsPerSegment + slot;
        live |= bool(oldBusy[at / 8] & (1u << (at % 8)));
      }
      if (!live && (kept || segment + 1 < oldCount)) {
        segmentRelease(segments[segment]);
        freed += SegmentBytes;
        continue;
      }
      segments[kept] = segments[segment];
      for (unsigned slot = 0; slot < SlotsPerSegment; ++slot) {
        const unsigned old = segment * SlotsPerSegment + slot;
        const unsigned at = kept * SlotsPerSegment + slot;
        if (oldBusy[old / 8] & (1u << (old % 8)))
          busy[at / 8] |= uint8_t(1u << (at % 8));
      }
      ++kept;
    }
    segmentCount = kept;
    for (unsigned i = kept; i < oldCount; ++i)
      segments[i] = nullptr;
    freeHead = nullptr;
    for (unsigned segment = 0; segment < kept; ++segment)
      for (unsigned slot = 0; slot < SlotsPerSegment; ++slot) {
        const unsigned at = segment * SlotsPerSegment + slot;
        if (busy[at / 8] & (1u << (at % 8)))
          continue;
        void *p = segments[segment] + slot * SlotBytes;
        std::memcpy(p, &freeHead, sizeof freeHead);
        freeHead = p;
      }
    error = "";
    return freed;
  }
  bool locate(const void *p, unsigned &index) const {
    const uintptr_t address = reinterpret_cast<uintptr_t>(p);
    for (unsigned i = 0; i < segmentCount; ++i) {
      uintptr_t base = reinterpret_cast<uintptr_t>(segments[i]);
      if (address >= base && address - base < SlotsPerSegment * SlotBytes &&
          (address - base) % SlotBytes == 0) {
        index = i * SlotsPerSegment + unsigned(address - base) / SlotBytes;
        return true;
      }
    }
    return false;
  }
  bool releaseAll() {
    if (usedSlots) {
      error = "Cold pool still owns live cells";
      return false;
    }
    for (unsigned i = 0; i < segmentCount; ++i) {
      segmentRelease(segments[i]);
      segments[i] = nullptr;
    }
    segmentCount = 0;
    freeHead = nullptr;
    owner = nullptr;
    segmentRelease = nullptr;
    evict = nullptr;
    busy.fill(0);
    error = "";
    return true;
  }
  bool prepare(unsigned count, Allocate allocate, Release release,
               Evict eviction, unsigned budget) {
    if (ready())
      return segmentCount == count;
    if (!count || count > MaxSegments || !allocate || !release ||
        uint64_t(count) * SegmentBytes > budget) {
      error = "Invalid cold pool segment budget";
      return false;
    }
    usedSlots = highSlots = requests = failures = evictions = invalidReleases =
        0;
    segmentRelease = release;
    evict = eviction;
    busy.fill(0);
    freeHead = nullptr;
    for (unsigned i = 0; i < count; ++i) {
      auto *p = static_cast<uint8_t *>(allocate(SegmentBytes));
      if (!p) {
        releaseAll();
        error = "Cannot reserve cold pool segments";
        return false;
      }
      segments[segmentCount++] = p;
    }
    for (unsigned i = 0; i < count; ++i)
      for (unsigned j = 0; j < SlotsPerSegment; ++j) {
        void *p = segments[i] + j * SlotBytes;
        std::memcpy(p, &freeHead, sizeof(freeHead));
        freeHead = p;
      }
    error = "";
    return true;
  }
  void *allocate(size_t bytes) {
    ++requests;
    if (!bytes || bytes > SlotBytes || !ready()) {
      ++failures;
      error = "Invalid cold pool cell allocation";
      return nullptr;
    }
    unsigned attempts = 0;
    while (!freeHead) {
      if (!evict || attempts++ >= MaxSlots || !evict()) {
        ++failures;
        error = "Cold pool capacity exhausted";
        return nullptr;
      }
      ++evictions;
    }
    void *p = freeHead;
    unsigned index = 0;
    if (!locate(p, index) || (busy[index / 8] & (1u << (index % 8)))) {
      ++failures;
      error = "Invalid cold pool free list";
      return nullptr;
    }
    std::memcpy(&freeHead, p, sizeof(freeHead));
    busy[index / 8] |= uint8_t(1u << (index % 8));
    ++usedSlots;
    highSlots = std::max(highSlots, usedSlots);
    error = "";
    return p;
  }
  bool release(void *p) {
    if (!p)
      return true;
    unsigned index = 0;
    if (!locate(p, index) || !(busy[index / 8] & (1u << (index % 8)))) {
      ++invalidReleases;
      error = "Invalid cold pool cell release";
      return false;
    }
    busy[index / 8] &= uint8_t(~(1u << (index % 8)));
    --usedSlots;
    std::memcpy(p, &freeHead, sizeof(freeHead));
    freeHead = p;
    return true;
  }
};
static_assert(sizeof(Block) <= SegmentPool::SlotBytes,
              "Cold chunk fits reserved cell");
inline SegmentPool pool;
inline void *poolAllocate(size_t bytes) { return pool.allocate(bytes); }
inline void poolRelease(void *p) { pool.release(p); }
inline unsigned poolResidentBytes() { return pool.residentBytes(); }
inline unsigned poolFixedBytes() { return sizeof(pool); }
template <unsigned Capacity = 96> struct Cache {
  Entry entries[Capacity]{};
  using Allocate = void *(*)(size_t);
  using Release = void (*)(void *);
  Allocate allocate = std::malloc;
  Release release = std::free;
  Entry *pinned = nullptr;
  unsigned (*residentCharge)() = nullptr;
  unsigned bytes = 0, clock = 0, hits = 0, stores = 0, allocationFailures = 0,
           workspacePeak = 0;
  const char *error = "";
  Cache() = default;
  Cache(const Cache &) = delete;
  Cache &operator=(const Cache &) = delete;
  ~Cache() { clear(); }
  unsigned allocatedBytes() const { return bytes; }
  unsigned residentBytes() const {
    return residentCharge ? residentCharge() : bytes;
  }
  unsigned fixedBytes() const { return sizeof(*this) + WorkspaceBytes; }
  static bool named(const char *s) { return s && *s; }
  static bool overlaps(const void *a, unsigned an, const void *b, unsigned bn) {
    const uintptr_t x = reinterpret_cast<uintptr_t>(a),
                    y = reinterpret_cast<uintptr_t>(b);
    return x <= y ? y - x < an : x - y < bn;
  }
  static bool matches(const Entry &e, const Key &k) {
    return e.blocks && e.id == k.id && e.x == k.x && e.y == k.y && e.w == k.w &&
           e.h == k.h && (named(e.file) == named(k.file)) &&
           (!named(k.file) || !std::strcmp(e.file, k.file));
  }
  Entry *find(const Key &k) {
    for (auto &e : entries)
      if (matches(e, k)) {
        e.age = ++clock;
        ++hits;
        return &e;
      }
    return nullptr;
  }
  Entry *oldest() {
    Entry *r = nullptr;
    for (auto &e : entries)
      if (&e != pinned && e.blocks && (!r || e.age < r->age))
        r = &e;
    return r;
  }
  bool reclaimResidentEntry() {
    if (residentCharge)
      return false;
    if (auto *e = oldest()) {
      erase(*e);
      return true;
    }
    return false;
  }
  void freeEntry(Entry &e) {
    auto *b = e.blocks;
    while (b) {
      auto *next = b->next;
      release(b);
      b = next;
    }
    if (e.file)
      release(e.file);
    e = Entry{};
  }
  void erase(Entry &e) {
    bytes -= e.allocatedBytes;
    freeEntry(e);
  }
  void clear() {
    pinned = nullptr;
    for (auto &e : entries)
      erase(e);
    error = "";
  }
  struct Arena {
    uint8_t *data;
    unsigned size, used = 0;
  };
  static voidpf zAllocate(voidpf opaque, uInt n, uInt size) {
    auto &a = *static_cast<Arena *>(opaque);
    if (!size || !n || n > a.size / size)
      return nullptr;
    unsigned count = n * size;
    uintptr_t base = reinterpret_cast<uintptr_t>(a.data);
    unsigned offset = unsigned(((base + a.used + 63) & ~uintptr_t(63)) - base);
    if (offset > a.size || count > a.size - offset)
      return nullptr;
    auto *p = a.data + offset;
    std::memset(p, 0, count);
    a.used = offset + count;
    return p;
  }
  static void zRelease(voidpf, voidpf) {}
  bool room(unsigned n, unsigned limit, unsigned pending) {
    if (residentCharge)
      return residentBytes() <= limit;
    while (uint64_t(bytes) + pending + n > limit) {
      auto *e = oldest();
      if (!e)
        return false;
      erase(*e);
    }
    return true;
  }
  Result save(const Key &key, const uint8_t *source, unsigned format,
              unsigned colors, void *workspace, unsigned workspaceBytes,
              unsigned byteLimit) {
    error = "";
    const unsigned rawBytes = format == 5 ? 66560 : 262144;
    if (!source || !workspace || workspaceBytes < WorkspaceBytes ||
        overlaps(source, rawBytes, workspace, workspaceBytes) ||
        (format != 3 && format != 5) ||
        (format == 5 && (!colors || colors > 256)) ||
        (key.id == UINT_MAX && !named(key.file)))
      return Result::Invalid;
    if (find(key))
      return Result::AlreadyPresent;
    Entry pending;
    pending.id = key.id;
    pending.x = key.x;
    pending.y = key.y;
    pending.w = key.w;
    pending.h = key.h;
    pending.format = format;
    pending.colors = colors;
    pending.rawBytes = rawBytes;
    pending.crc = uint32_t(crc32(0, source, rawBytes));
    if (named(key.file)) {
      size_t n = std::strlen(key.file) + 1;
      if (n > 4096)
        return Result::Invalid;
      if (!room(unsigned(n), byteLimit, 0))
        return Result::BudgetExceeded;
      pending.file = static_cast<char *>(allocate(n));
      if (!pending.file) {
        ++allocationFailures;
        return Result::AllocationFailed;
      }
      std::memcpy(pending.file, key.file, n);
      pending.allocatedBytes = unsigned(n);
    }
    Arena arena{static_cast<uint8_t *>(workspace), workspaceBytes};
    z_stream z{};
    z.zalloc = zAllocate;
    z.zfree = zRelease;
    z.opaque = &arena;
    int status =
        deflateInit2(&z, Z_BEST_SPEED, Z_DEFLATED, 13, 3, Z_DEFAULT_STRATEGY);
    if (status != Z_OK) {
      freeEntry(pending);
      error = "Cold zlib workspace/init failed";
      return Result::Invalid;
    }
    z.next_in = const_cast<Bytef *>(source);
    z.avail_in = rawBytes;
    Block *tail = nullptr;
    Result result = Result::Stored;
    for (;;) {
      if (pending.allocatedBytes + sizeof(Block) >= rawBytes) {
        result = Result::NotUseful;
        break;
      }
      if (!room(sizeof(Block), byteLimit, pending.allocatedBytes)) {
        result = Result::BudgetExceeded;
        break;
      }
      auto *b = static_cast<Block *>(allocate(sizeof(Block)));
      if (!b) {
        ++allocationFailures;
        result = Result::AllocationFailed;
        break;
      }
      b->next = nullptr;
      b->used = 0;
      if (tail)
        tail->next = b;
      else
        pending.blocks = b;
      tail = b;
      pending.allocatedBytes += sizeof(Block);
      z.next_out = b->data;
      z.avail_out = ChunkBytes;
      status = deflate(&z, Z_FINISH);
      b->used = ChunkBytes - z.avail_out;
      pending.encodedBytes += b->used;
      if (status == Z_STREAM_END)
        break;
      if (status != Z_OK || !b->used) {
        error = "Cold deflate failed";
        result = Result::Invalid;
        break;
      }
    }
    workspacePeak = std::max(workspacePeak, arena.used);
    deflateEnd(&z);
    if (result != Result::Stored) {
      freeEntry(pending);
      return result;
    }
    Entry *slot = nullptr;
    for (auto &e : entries)
      if (!e.blocks) {
        slot = &e;
        break;
      }
    if (!slot) {
      slot = oldest();
      if (!slot) {
        freeEntry(pending);
        return Result::BudgetExceeded;
      }
      erase(*slot);
    }
    pending.age = ++clock;
    pending.serial = ++stores;
    *slot = pending;
    bytes += pending.allocatedBytes;
    return Result::Stored;
  }
  bool decode(Entry &e, uint8_t *target, unsigned targetBytes, void *workspace,
              unsigned workspaceBytes) {
    error = "";
    if (!e.blocks || !target || targetBytes < e.rawBytes || !workspace ||
        workspaceBytes < WorkspaceBytes ||
        overlaps(target, e.rawBytes, workspace, workspaceBytes) ||
        (e.format != 3 && e.format != 5) ||
        e.rawBytes != (e.format == 5 ? 66560u : 262144u) || !e.encodedBytes ||
        e.encodedBytes >= e.rawBytes ||
        (e.format == 5 && (!e.colors || e.colors > 256))) {
      error = "Invalid cold texture decode";
      return false;
    }
    Arena arena{static_cast<uint8_t *>(workspace), workspaceBytes};
    z_stream z{};
    z.zalloc = zAllocate;
    z.zfree = zRelease;
    z.opaque = &arena;
    if (inflateInit(&z) != Z_OK) {
      error = "Cold inflate workspace/init failed";
      return false;
    }
    auto *b = e.blocks;
    unsigned supplied = 0, blocks = 0;
    int status = Z_OK;
    bool good = false;
    uint8_t overflow = 0;
    for (;;) {
      if (!z.avail_in) {
        if (!b || ++blocks > (e.encodedBytes + ChunkBytes - 1) / ChunkBytes ||
            !b->used || b->used > ChunkBytes ||
            b->used > e.encodedBytes - supplied)
          break;
        z.next_in = b->data;
        z.avail_in = b->used;
        supplied += b->used;
        b = b->next;
      }
      z.next_out = z.total_out < e.rawBytes ? target + z.total_out : &overflow;
      z.avail_out =
          z.total_out < e.rawBytes ? e.rawBytes - unsigned(z.total_out) : 1;
      uLong before = z.total_out;
      unsigned beforeIn = z.avail_in;
      status = inflate(&z, Z_NO_FLUSH);
      if (z.total_out > e.rawBytes)
        break;
      if (status == Z_STREAM_END) {
        good = z.total_out == e.rawBytes && !z.avail_in && !b &&
               supplied == e.encodedBytes;
        break;
      }
      if (status != Z_OK || (before == z.total_out && beforeIn == z.avail_in))
        break;
    }
    workspacePeak = std::max(workspacePeak, arena.used);
    inflateEnd(&z);
    good = good && uint32_t(crc32(0, target, e.rawBytes)) == e.crc;
    if (!good)
      error = "Cold texture inflate/length/CRC mismatch";
    else
      e.age = ++clock;
    return good;
  }
};
template <unsigned Capacity>
bool prepareColdPool(Cache<Capacity> &cache, unsigned segments,
                     SegmentPool::Allocate allocate,
                     SegmentPool::Release release, SegmentPool::Evict evict,
                     unsigned residentBudget) {
  if (cache.residentCharge == poolResidentBytes && pool.owner == &cache)
    return pool.ready() && pool.segmentCount == segments;
  if (cache.allocatedBytes() || (pool.owner && pool.owner != &cache)) {
    pool.error = "Clear cold entries before changing allocator ownership";
    return false;
  }
  if (!pool.prepare(segments, allocate, release, evict, residentBudget))
    return false;
  pool.owner = &cache;
  cache.allocate = poolAllocate;
  cache.release = poolRelease;
  cache.residentCharge = poolResidentBytes;
  return true;
}
template <unsigned Capacity> bool releaseColdPool(Cache<Capacity> &cache) {
  if (!pool.ready() && cache.residentCharge != poolResidentBytes)
    return true;
  if (pool.owner != &cache) {
    pool.error = "Cold pool belongs to another cache";
    return false;
  }
  cache.clear();
  if (!pool.releaseAll())
    return false;
  cache.allocate = std::malloc;
  cache.release = std::free;
  cache.residentCharge = nullptr;
  return true;
}
#if defined(__mips__)
static_assert(sizeof(Entry) == 60, "PSP cold Entry ABI");
static_assert(sizeof(Cache<96>) == 5804, "PSP cold Cache ABI");
#endif
} // namespace Cold
