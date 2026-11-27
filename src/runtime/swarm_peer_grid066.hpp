#pragma once

#include "game.hpp"

// Conservative peer broad phase. Exact collision and correction arithmetic
// remains in ChuckleRun, including ascending peer tie order and correction suffix.
class SwarmPeerGrid066 {
public:
    static constexpr unsigned Capacity=180,Words=6,Buckets=256;
    static constexpr float CellSize=64,CoordinateLimit=8192;
    struct Mask {
        uint32_t words[Words];
        template<class Visit> void each(Visit visit) const {
            for (unsigned word=0;word<Words;++word) {
                uint32_t bits=words[word];
                while (bits) {
                    const unsigned bit=static_cast<unsigned>(__builtin_ctz(bits));
                    bits&=bits-1;
                    visit(32*word+bit);
                }
            }
        }
    };

    bool build(const CircleCollider* bodies,unsigned count) {
        enabled=false;
        cacheValid=false;
        if (count>Capacity) return false;
        for (unsigned i=0;i<count;++i) {
            const auto& body=bodies[i];
            if (body.radius<=0) continue;
            if (body.radius!=16 || !supported(body.center)) return false;
        }
        for (auto& bucket:buckets) std::fill_n(bucket.words,Words,0u);
        for (unsigned i=0;i<count;++i) {
            if (bodies[i].radius>0) insert(i,bodies[i].center);
        }
        enabled=true;
        return true;
    }

    bool query(Vec2 center,Mask& result) {
        if (!enabled) { cacheValid=false; return false; }
        if (!supported(center)) {
            enabled=false;
            cacheValid=false;
            return false;
        }
        const int x=cell(center.x),y=cell(center.y);
        if (cacheValid && x==cachedX && y==cachedY) {
            result=cached;
            return true;
        }
        std::fill_n(result.words,Words,0u);
        // Original reach is 32+200/60 (<64). Fixed neighboring cells avoid
        // inward-rounded center +/- reach endpoints at float boundaries.
        for (int dy=-1;dy<=1;++dy) {
            for (int dx=-1;dx<=1;++dx) {
                const auto& bucket=buckets[hash(x+dx,y+dy)];
                for (unsigned word=0;word<Words;++word) result.words[word]|=bucket.words[word];
            }
        }
        cached=result;
        cachedX=x;
        cachedY=y;
        cacheValid=true;
        return true;
    }

    void remove(unsigned index,Vec2 before) {
        if (enabled) cacheValid=false;
        if (enabled) buckets[hash(cell(before.x),cell(before.y))].words[index/32]&=~(uint32_t{1}<<(index%32));
    }

    void update(unsigned index,Vec2 before,Vec2 after) {
        if (!enabled) return;
        if (!supported(after)) {
            enabled=false;
            cacheValid=false;
            return;
        }
        const unsigned oldBucket=hash(cell(before.x),cell(before.y));
        const unsigned newBucket=hash(cell(after.x),cell(after.y));
        if (oldBucket==newBucket) return;
        cacheValid=false;
        buckets[oldBucket].words[index/32]&=~(uint32_t{1}<<(index%32));
        insert(index,after);
    }

private:
    Mask buckets[Buckets];
    bool enabled=false;
    Mask cached{};
    int cachedX=0,cachedY=0;
    bool cacheValid=false;
    static bool supported(Vec2 point) {
        return std::isfinite(point.x) && std::isfinite(point.y) &&
               std::abs(point.x)<=CoordinateLimit && std::abs(point.y)<=CoordinateLimit;
    }
    static int cell(float coordinate) { return static_cast<int>(std::floor(coordinate/CellSize)); }
    static unsigned hash(int x,int y) {
        return (static_cast<uint32_t>(x)*0x8da6b343u ^ static_cast<uint32_t>(y)*0xd8163841u)&(Buckets-1);
    }
    void insert(unsigned index,Vec2 center) {
        buckets[hash(cell(center.x),cell(center.y))].words[index/32]|=uint32_t{1}<<(index%32);
    }
};
static_assert(sizeof(SwarmPeerGrid066::Mask)*SwarmPeerGrid066::Buckets==6144);
