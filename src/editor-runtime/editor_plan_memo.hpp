#pragma once
#include "editor_sample_plan.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
namespace EditorPlanMemo {
inline constexpr size_t BudgetBytes = 208 * 1024, MaxPixels = 4096,
                        MaxBlockBytes = MaxPixels * 4;
inline constexpr unsigned Capacity = 256, MinPixels = 160, Buckets = 512;
// Generic cache-admission policy only; every rejected shape uses the old
// builder.
inline bool admitShape(unsigned w, unsigned h) {
  if (!w || !h || w > 256 || h > 256)
    return false;
  const uint64_t pixels = uint64_t(w) * h;
  return pixels >= MinPixels && pixels <= MaxPixels &&
         std::max(w, h) * 4u <= std::min(w, h) * 5u;
}
struct Key {
  std::array<uint32_t, 22> words{};
  template <class Quad>
  static Key from(const Quad &q, int left, int top, unsigned w, unsigned h) {
    Key key;
    static_assert(sizeof(q[0].x) == 4 && sizeof(q[0].y) == 4 &&
                      sizeof(q[0].u) == 4 && sizeof(q[0].v) == 4,
                  "float32 geometry key");
    for (unsigned i = 0; i < 4; ++i) {
      std::memcpy(&key.words[i * 4], &q[i].x, 4);
      std::memcpy(&key.words[i * 4 + 1], &q[i].y, 4);
      std::memcpy(&key.words[i * 4 + 2], &q[i].u, 4);
      std::memcpy(&key.words[i * 4 + 3], &q[i].v, 4);
    }
    key.words[16] = uint32_t(left);
    key.words[17] = uint32_t(top);
    key.words[18] = w;
    key.words[19] = h;
    key.words[20] = key.words[21] = 256;
    return key;
  }
};
static_assert(sizeof(Key) == 88, "full unnormalized geometry key");
struct Vertex {
  float x, y, u, v;
};
inline bool buildExact(const Key &key, uint32_t *out, void *) {
  std::array<Vertex, 4> q{};
  std::memcpy(q.data(), key.words.data(), 64);
  int32_t left, top;
  std::memcpy(&left, &key.words[16], 4);
  std::memcpy(&top, &key.words[17], 4);
  return EditorSamplePlan::build(q, int(left), int(top),
                                 unsigned(key.words[18]),
                                 unsigned(key.words[19]), 256, 256, out);
}
struct Allocator {
  void *(*allocate)(size_t, void *) = nullptr;
  void (*release)(void *, void *) = nullptr;
  void *context = nullptr;
};
using Hasher = uint32_t (*)(const Key &, void *);
using Builder = bool (*)(const Key &, uint32_t *, void *);
inline uint32_t hashExact(const Key &key, void *) {
  const auto *bytes = reinterpret_cast<const uint8_t *>(key.words.data());
  uint32_t h = 2166136261u;
  for (unsigned i = 0; i < sizeof(Key); ++i) {
    h ^= bytes[i];
    h *= 16777619u;
  }
  return h;
}
struct Stats {
  uint64_t hits = 0, misses = 0, stores = 0, evictions = 0,
           allocationFailures = 0, bypasses = 0, cancelled = 0,
           buildFailures = 0, clears = 0;
  size_t peakBytes = 0;
};
class Cache {
  static constexpr uint16_t None = UINT16_MAX;
  struct Entry {
    Key key;
    uint32_t *plan = nullptr;
    unsigned bytes = 0;
    uint32_t hash = 0;
    uint16_t next = None;
    uint64_t age = 0;
  };
  std::array<Entry, Capacity> entries{};
  std::array<uint16_t, Buckets> buckets;
  Allocator allocator;
  Hasher hasher;
  void *hashContext;
  Builder builder;
  void *buildContext;
  Key pendingKey;
  size_t ownedBytes = 0, pendingBytes = 0;
  unsigned count = 0;
  uint64_t stamp = 0, epoch = 1;
  bool busy = false;
  Stats data;
  void *allocate(size_t bytes) {
    return allocator.allocate ? allocator.allocate(bytes, allocator.context)
                              : std::malloc(bytes);
  }
  void release(void *p) {
    if (allocator.release)
      allocator.release(p, allocator.context);
    else
      std::free(p);
  }
  void erase(unsigned index) {
    auto &e = entries[index];
    auto *link = &buckets[e.hash & (Buckets - 1)];
    while (*link != None && *link != index)
      link = &entries[*link].next;
    if (*link == index)
      *link = e.next;
    release(e.plan);
    ownedBytes -= e.bytes;
    --count;
    e = Entry{};
  }
  unsigned oldest() const {
    unsigned result = Capacity;
    for (unsigned i = 0; i < Capacity; ++i)
      if (entries[i].plan &&
          (result == Capacity || entries[i].age < entries[result].age))
        result = i;
    return result;
  }
  void touch(Entry &e) {
    if (stamp == UINT64_MAX) {
      for (auto &x : entries)
        x.age = 0;
      stamp = 0;
    }
    e.age = ++stamp;
  }
  static bool valid(const Key &key, size_t capacity) {
    const auto &w = key.words;
    uint64_t count = uint64_t(w[18]) * w[19];
    if (!admitShape(unsigned(w[18]), unsigned(w[19])) || count < MinPixels ||
        count > MaxPixels || count > capacity || w[20] != 256 || w[21] != 256)
      return false;
    for (unsigned i = 0; i < 16; ++i) {
      float f;
      std::memcpy(&f, &w[i], 4);
      if (!std::isfinite(f))
        return false;
    }
    return true;
  }

public:
  explicit Cache(Allocator a = {}, Hasher hash = hashExact, void *hc = nullptr,
                 Builder build = buildExact, void *bc = nullptr)
      : allocator(a), hasher(hash ? hash : hashExact), hashContext(hc),
        builder(build ? build : buildExact), buildContext(bc) {
    buckets.fill(None);
    data.peakBytes = sizeof(Cache);
  }
  Cache(const Cache &) = delete;
  Cache &operator=(const Cache &) = delete;
  ~Cache() { clear(); }
  static constexpr size_t fixedBytes() { return sizeof(Cache); }
  static constexpr size_t committedLimit() {
    return BudgetBytes - sizeof(Cache) - MaxBlockBytes;
  }
  size_t accountedBytes() const {
    return sizeof(Cache) + ownedBytes + pendingBytes;
  }
  size_t committedBytes() const { return ownedBytes; }
  unsigned entryCount() const { return count; }
  const Stats &stats() const { return data; }
  void clear() {
    for (unsigned i = 0; i < Capacity; ++i)
      if (entries[i].plan)
        erase(i);
    buckets.fill(None);
    stamp = 0;
    ++epoch;
    ++data.clears;
  }
  bool copy(const Key &key, uint32_t *out, size_t capacity) {
    static_assert(sizeof(Cache) + MaxBlockBytes < BudgetBytes,
                  "bounded plan cache metadata/pending");
    if (busy || !out || !valid(key, capacity) ||
        bool(allocator.allocate) != bool(allocator.release)) {
      ++data.bypasses;
      return false;
    }
    // Copy every key bit before any user callback; no borrowed geometry
    // survives.
    pendingKey = key;
    busy = true;
    const uint64_t started = epoch;
    const uint32_t hash = hasher(pendingKey, hashContext);
    if (started != epoch) {
      busy = false;
      ++data.cancelled;
      return false;
    }
    for (uint16_t index = buckets[hash & (Buckets - 1)]; index != None;
         index = entries[index].next) {
      auto &e = entries[index];
      if (e.hash == hash &&
          !std::memcmp(e.key.words.data(), pendingKey.words.data(),
                       sizeof(Key))) {
        std::memcpy(out, e.plan, e.bytes);
        touch(e);
        ++data.hits;
        busy = false;
        return true;
      }
    }
    ++data.misses;
    const unsigned bytes =
        unsigned(pendingKey.words[18] * pendingKey.words[19] * 4);
    auto *pending = static_cast<uint32_t *>(allocate(bytes));
    if (!pending) {
      ++data.allocationFailures;
      busy = false;
      return false;
    }
    pendingBytes = bytes;
    data.peakBytes = std::max(data.peakBytes, accountedBytes());
    bool success =
        started == epoch && builder(pendingKey, pending, buildContext);
    if (started != epoch) {
      success = false;
      ++data.cancelled;
    } else if (!success)
      ++data.buildFailures;
    if (!success) {
      release(pending);
      pendingBytes = 0;
      busy = false;
      return false;
    }
    // Pending storage was reserved within the hard208KiB cap before evicting
    // anything. A denied/failed miss preserves every previously cached plan.
    while (count == Capacity || ownedBytes + bytes > committedLimit()) {
      unsigned victim = oldest();
      if (victim == Capacity) {
        release(pending);
        pendingBytes = 0;
        busy = false;
        return false;
      }
      erase(victim);
      ++data.evictions;
    }
    unsigned slot = 0;
    while (entries[slot].plan)
      ++slot;
    auto &e = entries[slot];
    e.key = pendingKey;
    e.plan = pending;
    e.bytes = bytes;
    e.hash = hash;
    e.next = buckets[hash & (Buckets - 1)];
    buckets[hash & (Buckets - 1)] = uint16_t(slot);
    touch(e);
    ownedBytes += bytes;
    pendingBytes = 0;
    ++count;
    ++data.stores;
    std::memcpy(out, pending, bytes);
    busy = false;
    return true;
  }
};
} // namespace EditorPlanMemo
