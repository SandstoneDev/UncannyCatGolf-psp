#pragma once

#include "game.hpp"

enum class SecretEvent { None, Start, Key };

struct SecretRunner {
    static constexpr Vec2 Hint{-448,-160};
    static constexpr float StartX=-672.0f, EndX=672.0f, Y=-96.0f;
    static constexpr unsigned RunTicks=600;
    static constexpr float SpriteScale=0.674f;
    static constexpr Vec2 HitHalfSize{47.4778f*SpriteScale/2,71.2166f*SpriteScale/2};

    bool available=false,collected=false;
    unsigned startedAt=0;

    Vec2 position(unsigned tick) const {
        const unsigned elapsed=startedAt?std::min(tick-startedAt,RunTicks):0;
        return {StartX+(EndX-StartX)*elapsed/RunTicks,Y};
    }

    SecretEvent step(Vec2 player,float radius,unsigned tick) {
        if (!available || collected) return SecretEvent::None;
        if (!startedAt) {
            if (!overlaps(player,radius,Hint,{16,16})) return SecretEvent::None;
            startedAt=tick;
            return SecretEvent::Start;
        }
        if (!overlaps(player,radius,position(tick),HitHalfSize)) return SecretEvent::None;
        collected=true;
        return SecretEvent::Key;
    }

private:
    static bool overlaps(Vec2 player,float radius,Vec2 center,Vec2 half) {
        const Vec2 delta=player-center;
        const Vec2 distance{std::max(std::abs(delta.x)-half.x,0.0f),std::max(std::abs(delta.y)-half.y,0.0f)};
        return dot(distance,distance)<=radius*radius;
    }
};
