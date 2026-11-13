#pragma once

#include "frontier18501_mechanics.hpp"
#include "elysian_rocks039.hpp"

struct HorseBroadcast039 {
    enum class Cue { None, MuteMusic, Alarm };
    static constexpr unsigned AdvanceTick=421;
    unsigned ticks=0;
    bool started=false;

    bool active() const { return started; }
    bool ready() const { return started && ticks>=AdvanceTick; }
    unsigned elapsedMs() const { return ticks*1000/60; }

    Cue advance() {
        if (ticks>=AdvanceTick) return Cue::None;
        ++ticks;
        if (ticks==60) return Cue::MuteMusic;
        if (ticks==120) return Cue::Alarm;
        return Cue::None;
    }
};

inline WallContact sweepElysianRocks039(Vec2 position,Vec2 motion,float radius) {
    const auto first=sweepElysianRockGroup<ElysianRockOne039,ElysianRockOneOffsets039>(position,motion,radius);
    const auto second=sweepElysianRockGroup<ElysianRockTwo039,ElysianRockTwoOffsets039>(position,motion,radius);
    return second.time<first.time?second:first;
}

inline void stepElysian039(Game& game,const Course& course,HorseBroadcast039& broadcast,bool fakeGoal) {
    game.step(nullptr,0,{},nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,nullptr,nullptr,
              nullptr,1,{},false,fakeGoal?nullptr:sweepElysianRocks039);
    if (!fakeGoal) return;
    const Vec2 delta=game.position-course.goal;
    const float radius=course.playerRadius+course.goalRadius;
    if (dot(delta,delta)<=radius*radius) {
        broadcast.started=true;
        game.won=true;
        game.velocity={};
    }
}
