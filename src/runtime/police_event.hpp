#pragma once

#include "game.hpp"

struct PoliceRun {
    unsigned triggeredAt=0;

    bool trigger(Vec2 player,float radius,unsigned tick) {
        if (triggeredAt) return false;
        const Vec2 closest{std::clamp(player.x,-128.0f,192.0f),
                           std::clamp(player.y,-320.0f,64.0f)};
        const Vec2 offset=player-closest;
        if (dot(offset,offset)>radius*radius) return false;
        triggeredAt=tick;
        return true;
    }
};
