#pragma once

#include "game.hpp"

class NukeRun {
public:
    static constexpr unsigned Capacity=8;
    static constexpr float Radius=16.0f;
    unsigned pickedAt[Capacity]{};

    bool has(unsigned index) const { return pickedAt[index]!=0; }

    bool step(const Vec2* positions,unsigned count,Vec2 player,float radius,unsigned tick) {
        bool picked=false;
        const float range=radius+Radius;
        for (unsigned i=0;i<count;++i) {
            const Vec2 offset=positions[i]-player;
            if (has(i) || dot(offset,offset)>range*range) continue;
            pickedAt[i]=tick;
            picked=true;
        }
        return picked;
    }
};
