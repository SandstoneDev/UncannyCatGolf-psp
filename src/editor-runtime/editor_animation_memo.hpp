#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace EditorAnimationMemo {
// No base sprite image/frame is cached: invalid/empty animations leave both
// caller values untouched. Every result bit is explicit (no padding).
struct Selection { int image; uint32_t overrideImage; };
static_assert(sizeof(Selection) == 8, "explicit selection bits");

template<class Animation, class Frames>
inline Selection select(const Animation& a, const Frames& frames, double seconds) {
    // Keep the original expression, evaluation order and double intermediates.
    double length = 0;
    for (unsigned j = 0; j < a.count; ++j)
        length += frames[a.first + j].duration;
    if (a.count && length > 0) {
        double time = seconds * a.speed;
        time = a.loop ? std::fmod(time, length) : std::min(time, length - .00001);
        unsigned at = 0;
        while (at + 1 < a.count && time >= frames[a.first + at].duration) {
            time -= frames[a.first + at].duration;
            ++at;
        }
        return {frames[a.first + at].image, 1};
    }
    return {0, 0};
}

// One immutable animation/frame table per cache. Full animation ID and binary64
// seconds are compared; a direct-map collision only computes and replaces.
// Exceptional/out-of-range times always execute the original expression.
template<unsigned Capacity = 64> class Cache {
    static_assert(Capacity && !(Capacity & (Capacity - 1)), "power-of-two slots");
    struct Entry {
        uint64_t secondsBits = 0;
        unsigned animationId = 0;
        Selection selection{0, 0};
        bool valid = false;
    };
    std::array<Entry, Capacity> entries{};
public:
    static constexpr size_t fixedBytes() { return sizeof(Cache); }
    void clear() { for (auto& entry : entries) entry.valid = false; }
    template<class Compute>
    Selection get(unsigned animationId, double seconds, const Compute& compute) {
        if (!(seconds >= -1048576. && seconds <= 1048576.)) return compute();
        static_assert(sizeof(double) == sizeof(uint64_t), "binary64 seconds");
        uint64_t bits;
        uint32_t words[2];
        std::memcpy(&bits, &seconds, sizeof bits);
        std::memcpy(words, &seconds, sizeof words);
        // Only 32-bit arithmetic in the hash; key comparison retains all bits.
        uint32_t hash = words[0] ^ (words[1] * 0x9e3779b1u) ^ (animationId * 0x85ebca6bu);
        hash ^= hash >> 16;
        auto& entry = entries[hash & (Capacity - 1)];
        if (entry.valid && entry.animationId == animationId && entry.secondsBits == bits)
            return entry.selection;
        const Selection selection = compute();
        entry.secondsBits = bits;
        entry.animationId = animationId;
        entry.selection = selection;
        entry.valid = true;
        return selection;
    }
};
}
