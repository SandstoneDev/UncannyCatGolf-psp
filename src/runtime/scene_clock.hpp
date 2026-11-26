#pragma once
#include <algorithm>
#include <cstdint>

// Convert an existing capped UI delta without discarding a fraction every frame.
// This does not pace frames or change the shared simulation clock.
struct SceneMilliseconds {
    unsigned remainderUs=0;
    unsigned take(uint64_t deltaUs) {
        const unsigned credit=static_cast<unsigned>(std::min<uint64_t>(deltaUs,100000))+remainderUs;
        remainderUs=credit%1000;
        return credit/1000;
    }
    void reset() { remainderUs=0; }
};
