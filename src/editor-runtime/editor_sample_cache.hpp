#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
namespace EditorSampleCache {
#ifndef UCG_EDITOR_SAMPLE_CACHE_BYTES
#define UCG_EDITOR_SAMPLE_CACHE_BYTES (256 * 1024)
#endif
#ifndef UCG_EDITOR_SAMPLE_CACHE_CAPACITY
#define UCG_EDITOR_SAMPLE_CACHE_CAPACITY 96
#endif
inline constexpr size_t BudgetBytes = UCG_EDITOR_SAMPLE_CACHE_BYTES, Capacity = UCG_EDITOR_SAMPLE_CACHE_CAPACITY,
                        MaxPixels = 4096, MaxKeyBytes = 4096;
inline constexpr size_t MaxBlockBytes =
    ((MaxKeyBytes + 1 + 3) & ~size_t(3)) + MaxPixels * 8;
inline constexpr uint32_t Outside = UINT32_MAX;
enum class Kind : uint8_t { Bank, Custom };
struct Key {
  Kind kind = Kind::Bank;
  uint32_t pageId = 0;
  const char *name = nullptr;
  size_t nameBytes = 0;
  uint32_t x = 0, y = 0, w = 0, h = 0;
  static Key bank(uint32_t id) {
    Key k;
    k.pageId = id;
    return k;
  }
  static Key custom(const char *n, size_t bytes, uint32_t x, uint32_t y,
                    uint32_t w, uint32_t h) {
    Key k;
    k.kind = Kind::Custom;
    k.name = n;
    k.nameBytes = bytes;
    k.x = x;
    k.y = y;
    k.w = w;
    k.h = h;
    return k;
  }
};
struct Request {
  Key key;
  unsigned width = 0, height = 0;
  const uint32_t *plan = nullptr;
  size_t count = 0;
};
// Getter must write ALL unlit RGBA words, including Outside and alpha0RGB.
// Its input plan is cache-owned; caller decode scratch may be overwritten.
using SourceGetter = bool (*)(void *, const uint32_t *, size_t, uint32_t *);
// Hash callbacks are pure and only accelerate lookup, never establish equality.
using HashFunction = uint32_t (*)(void *, const Key &, unsigned, unsigned,
                                  const uint32_t *, size_t);
using Allocate = void *(*)(size_t, void *);
using Release = void (*)(void *, void *);
inline void *defaultAllocate(size_t n, void *) { return std::malloc(n); }
inline void defaultRelease(void *p, void *) { std::free(p); }
struct Allocator {
  Allocate allocate = defaultAllocate;
  Release release = defaultRelease;
  void *context = nullptr;
};
enum class Status {
  Hit,
  Stored,
  InvalidRequest,
  Oversize,
  AllocationFailure,
  SourceFailure,
  EpochChanged,
  Busy
};
struct Result {
  Status status = Status::InvalidRequest;
  const uint32_t *plan = nullptr;
  const uint32_t *rgba = nullptr;
  size_t count = 0;
  bool hit = false;
  bool ready() const {
    return status == Status::Hit || status == Status::Stored;
  }
  explicit operator bool() const { return ready(); }
};
struct Stats {
  uint64_t hits = 0, misses = 0, stores = 0, evictions = 0,
           allocationFailures = 0, sourceFailures = 0, bypasses = 0, clears = 0,
           epochChanges = 0;
  size_t peakBytes = 0;
};
template<size_t ByteBudget=BudgetBytes,size_t Slots=Capacity>
class BasicCache {
  struct Entry {
    void *block = nullptr;
    size_t bytes = 0;
    uint64_t stamp = 0;
    uint32_t hash = 0, pageId = 0, nameBytes = 0, x = 0, y = 0, w = 0, h = 0,
             planOffset = 0, rgbaOffset = 0;
    uint16_t width = 0, height = 0, count = 0;
    Kind kind = Kind::Bank;
  };

public:
  explicit BasicCache(Allocator a = {}, HashFunction hash = nullptr,
                 void *hashContext = nullptr)
      : allocator(a), hasher(hash), hashContext(hashContext) {
    data.peakBytes = sizeof(BasicCache);
  }
  BasicCache(const BasicCache &) = delete;
  BasicCache &operator=(const BasicCache &) = delete;
  ~BasicCache() { clear(); }
  // Allocator/release/hash/getter callbacks must not throw. A getter may clear
  // this cache; that cancels publication but does not free its owned input.
  Result get(const Request &r, SourceGetter getter, void *context) {
    if (busy) {
      ++data.bypasses;
      return failure(Status::Busy);
    }
    const Status validation = validate(r);
    if (validation != Status::Stored || !getter) {
      ++data.bypasses;
      return failure(!getter ? Status::InvalidRequest : validation);
    }
    const uint32_t hash =
        hasher ? hasher(hashContext, r.key, r.width, r.height, r.plan, r.count)
               : defaultHash(r.key, r.width, r.height, r.plan, r.count);
    for (auto &e : entries)
      if (e.block && e.hash == hash && equal(e, r)) {
        ++data.hits;
        touch(e);
        return result(e, Status::Hit);
      }
    ++data.misses;
    if (!allocator.allocate || !allocator.release) {
      ++data.allocationFailures;
      return failure(Status::AllocationFailure);
    }
    BusyScope guard(busy);
    const uint64_t sourceEpoch = epoch;
    Entry candidate;
    candidate.kind = r.key.kind;
    candidate.hash = hash;
    candidate.pageId = r.key.pageId;
    candidate.x = r.key.x;
    candidate.y = r.key.y;
    candidate.w = r.key.w;
    candidate.h = r.key.h;
    candidate.nameBytes =
        r.key.kind == Kind::Custom ? uint32_t(r.key.nameBytes) : 0;
    candidate.width = uint16_t(r.width);
    candidate.height = uint16_t(r.height);
    candidate.count = uint16_t(r.count);
    candidate.planOffset =
        candidate.kind == Kind::Custom
            ? uint32_t((r.key.nameBytes + 1 + 3) & ~size_t(3))
            : 0;
    candidate.rgbaOffset = candidate.planOffset + uint32_t(r.count * 4);
    candidate.bytes = candidate.rgbaOffset + r.count * 4;
    // Committed storage always leaves MaxBlockBytes of pending headroom.
    if (candidate.bytes > MaxBlockBytes ||
        accountedBytes() > ByteBudget - candidate.bytes) {
      ++data.bypasses;
      return failure(Status::Oversize);
    }
    candidate.block = allocator.allocate(candidate.bytes, allocator.context);
    if (!candidate.block) {
      ++data.allocationFailures;
      return failure(Status::AllocationFailure);
    }
    pendingBytes = candidate.bytes;
    peak();
    if (reinterpret_cast<uintptr_t>(candidate.block) % alignof(uint32_t)) {
      discard(candidate);
      ++data.allocationFailures;
      return failure(Status::AllocationFailure);
    }
    if (sourceEpoch != epoch) {
      discard(candidate);
      ++data.epochChanges;
      return failure(Status::EpochChanged);
    }
    if (candidate.kind == Kind::Custom) {
      std::memcpy(candidate.block, r.key.name, r.key.nameBytes);
      static_cast<char *>(candidate.block)[r.key.nameBytes] = 0;
    }
    std::memcpy(plan(candidate), r.plan, r.count * 4);
    // No reads of caller key/plan/request are needed after the decoder
    // callback.
    const bool decoded =
        getter(context, plan(candidate), candidate.count, rgba(candidate));
    if (!decoded)
      ++data.sourceFailures;
    if (sourceEpoch != epoch) {
      discard(candidate);
      ++data.epochChanges;
      return failure(Status::EpochChanged);
    }
    if (!decoded) {
      discard(candidate);
      return failure(Status::SourceFailure);
    }
    while (count >= Slots ||
           ownedBytes > committedLimit() - candidate.bytes) {
      const size_t victim = oldest();
      if (victim == Slots) {
        discard(candidate);
        ++data.bypasses;
        return failure(Status::Oversize);
      }
      erase(entries[victim]);
      ++data.evictions;
    }
    for (auto &e : entries)
      if (!e.block) {
        e = candidate;
        ownedBytes += candidate.bytes;
        pendingBytes = 0;
        ++count;
        touch(e);
        ++data.stores;
        return result(e, Status::Stored);
      }
    discard(candidate);
    ++data.bypasses;
    return failure(Status::Oversize);
  }
  // Explicit source-epoch invalidation. During get(), pending storage remains
  // alive/accounted until its callback returns; committed old pointers expire.
  void clear() {
    for (auto &e : entries)
      if (e.block)
        erase(e);
    stamp = 0;
    ++epoch;
    ++data.clears;
  }
  size_t accountedBytes() const {
    return sizeof(BasicCache) + ownedBytes + pendingBytes;
  }
  size_t committedBytes() const { return ownedBytes; }
  size_t entryCount() const { return count; }
  static constexpr size_t fixedBytes() { return sizeof(BasicCache); }
  static constexpr size_t committedLimit() {
    return ByteBudget - sizeof(BasicCache) - MaxBlockBytes;
  }
  const Stats &stats() const { return data; }

private:
  Entry entries[Slots]{};
  Allocator allocator;
  HashFunction hasher = nullptr;
  void *hashContext = nullptr;
  size_t ownedBytes = 0, pendingBytes = 0, count = 0;
  uint64_t stamp = 0, epoch = 1;
  bool busy = false;
  Stats data{};
  struct BusyScope {
    bool &busy;
    explicit BusyScope(bool &b) : busy(b) { busy = true; }
    ~BusyScope() { busy = false; }
  };
  static Result failure(Status status) {
    Result r;
    r.status = status;
    return r;
  }
  static uint32_t *plan(const Entry &e) {
    return reinterpret_cast<uint32_t *>(static_cast<unsigned char *>(e.block) +
                                        e.planOffset);
  }
  static uint32_t *rgba(const Entry &e) {
    return reinterpret_cast<uint32_t *>(static_cast<unsigned char *>(e.block) +
                                        e.rgbaOffset);
  }
  static Result result(const Entry &e, Status status) {
    return {status, plan(e), rgba(e), e.count, status == Status::Hit};
  }
  static Status validate(const Request &r) {
    if (r.width > 256 || r.height > 256 || r.count > MaxPixels)
      return Status::Oversize;
    if (!r.width || !r.height || !r.plan ||
        r.count != size_t(r.width) * r.height)
      return Status::InvalidRequest;
    if (r.key.kind == Kind::Custom) {
      if (r.key.nameBytes > MaxKeyBytes)
        return Status::Oversize;
      if (!r.key.name || !r.key.nameBytes ||
          std::memchr(r.key.name, 0, r.key.nameBytes))
        return Status::InvalidRequest;
    } else if (r.key.kind != Kind::Bank)
      return Status::InvalidRequest;
    for (size_t i = 0; i < r.count; ++i)
      if (r.plan[i] != Outside && r.plan[i] >= 65536)
        return Status::InvalidRequest;
    return Status::Stored;
  }
  static bool equal(const Entry &e, const Request &r) {
    if (e.kind != r.key.kind || e.width != r.width || e.height != r.height ||
        e.count != r.count)
      return false;
    if (e.kind == Kind::Bank) {
      if (e.pageId != r.key.pageId)
        return false;
    } else if (e.nameBytes != r.key.nameBytes || e.x != r.key.x ||
               e.y != r.key.y || e.w != r.key.w || e.h != r.key.h ||
               std::memcmp(e.block, r.key.name, e.nameBytes))
      return false;
    return std::memcmp(plan(e), r.plan, r.count * 4) == 0;
  }
  static void mixByte(uint32_t &h, uint8_t b) { h = (h ^ b) * 16777619u; }
  static void mixWord(uint32_t &h, uint32_t w) {
    for (unsigned i = 0; i < 4; ++i) {
      mixByte(h, uint8_t(w));
      w >>= 8;
    }
  }
  static uint32_t defaultHash(const Key &key, unsigned width, unsigned height,
                              const uint32_t *map, size_t n) {
    uint32_t h = 2166136261u;
    mixWord(h, uint32_t(key.kind));
    mixWord(h, width);
    mixWord(h, height);
    if (key.kind == Kind::Bank)
      mixWord(h, key.pageId);
    else {
      mixWord(h, uint32_t(key.nameBytes));
      for (size_t i = 0; i < key.nameBytes; ++i)
        mixByte(h, uint8_t(key.name[i]));
      for (uint32_t v : {key.x, key.y, key.w, key.h})
        mixWord(h, v);
    }
    for (size_t i = 0; i < n; ++i)
      mixWord(h, map[i]);
    return h;
  }
  void peak() { data.peakBytes = std::max(data.peakBytes, accountedBytes()); }
  void discard(Entry &e) {
    void *p = e.block;
    e.block = nullptr;
    pendingBytes = 0;
    allocator.release(p, allocator.context);
  }
  void erase(Entry &e) {
    void *p = e.block;
    const size_t bytes = e.bytes;
    e = {};
    ownedBytes -= bytes;
    --count;
    allocator.release(p, allocator.context);
  }
  size_t oldest() const {
    size_t chosen = Slots;
    for (size_t i = 0; i < Slots; ++i)
      if (entries[i].block &&
          (chosen == Slots || entries[i].stamp < entries[chosen].stamp))
        chosen = i;
    return chosen;
  }
  void touch(Entry &e) {
    if (stamp == UINT64_MAX) {
      // Overflow is practically unreachable, but rank compression preserves
      // exact LRU order without wraparound assumptions or allocation.
      uint64_t ranks[Slots]{};
      uint64_t maximum = 0;
      for (size_t i = 0; i < Slots; ++i)
        if (entries[i].block) {
          uint64_t rank = 1;
          for (const auto &other : entries)
            if (other.block && other.stamp < entries[i].stamp)
              ++rank;
          ranks[i] = rank;
          maximum = std::max(maximum, rank);
        }
      for (size_t i = 0; i < Slots; ++i)
        if (entries[i].block)
          entries[i].stamp = ranks[i];
      stamp = maximum;
    }
    e.stamp = ++stamp;
  }
};
using Cache=BasicCache<>;
static_assert(MaxBlockBytes == 36868, "sample pending bound changed");
static_assert(sizeof(Cache) + MaxBlockBytes * 2 < BudgetBytes,
              "sample cache metadata exceeds bounded budget");
} // namespace EditorSampleCache
