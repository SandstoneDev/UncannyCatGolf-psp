#pragma once

#include "cosmic_goal_path051.hpp"

struct MovingGoal051 {
    enum class Face { Normal,Laugh,Cry };
    static constexpr unsigned LastTick=std::size(CosmicGoalPath613)-1;
    unsigned pathTick=0,changedAt=0;
    Face face=Face::Normal;

    Vec2 position() const { return CosmicGoalPath613[pathTick]; }

    bool advance(unsigned tick) {
        if (face!=Face::Normal) return false;
        pathTick=std::min(tick,LastTick);
        if (pathTick<LastTick) return false;
        face=Face::Laugh;
        changedAt=tick;
        return true;
    }

    bool cry(unsigned tick) {
        if (face==Face::Cry) return false;
        face=Face::Cry;
        changedAt=tick;
        return true;
    }

    bool flipLegs() const {
        return pathTick<203 || (pathTick>=483 && pathTick<591) || pathTick>=790;
    }
};

inline bool touchesCosmicKey613(Vec2 point,float radius) {
    const auto local=point-CosmicKeyTrigger613.center;
    const float x=std::max(0.0f,std::abs(local.x)-CosmicKeyTrigger613.halfSize.x);
    const float y=std::max(0.0f,std::abs(local.y)-CosmicKeyTrigger613.halfSize.y);
    return x*x+y*y<=radius*radius;
}
