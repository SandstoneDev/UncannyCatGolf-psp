#pragma once

#include "game.hpp"

struct MarcyEvents {
    bool started=false,intercom=false,cleaned=false,key=false;
};

struct MarcyRun {
    static constexpr Vec2 Hint{-224,112};
    static constexpr Vec2 Trigger{-240,128};
    static constexpr Vec2 TriggerHalfSize{16,32};
    static constexpr Vec2 SpillRest{-416,160};
    static constexpr float SpillHalfSize=32;
    static constexpr unsigned MaxCleans=10;

    unsigned startedAt=0,finishedAt=0,cleans=0;
    Vec2 spillCenter=SpillRest;

    void restart(bool enabled) {
        *this={};
        available=enabled;
    }

    bool hintVisible() const { return available; }
    bool spillVisible() const { return startedAt && !finishedAt; }
    float spillScale() const { return 1.0f-0.085f*cleans; }

    MarcyEvents step(Vec2 player,float radius,unsigned tick) {
        MarcyEvents events{};
        if (!available) return events;
        if (!startedAt) {
            if (overlaps(player,radius,Trigger,TriggerHalfSize)) {
                startedAt=tick;
                spillCenter=Trigger;
                events.started=true;
            }
            return events;
        }
        if (finishedAt) {
            const unsigned age=tick-finishedAt;
            events.intercom=age==2;
            events.key=age==RewardDelay;
            return events;
        }

        events.intercom=tick-startedAt==SlideTicks;
        const float half=SpillHalfSize*spillScale();
        const bool inside=overlaps(player,radius,spillCenter,{half,half});
        if (inside && !spillOverlapping) {
            if (cleans<MaxCleans) {
                ++cleans;
                events.cleaned=true;
            } else {
                finishedAt=tick;
                spillCenter=SpillRest;
            }
        }
        spillOverlapping=inside;
        return events;
    }

    unsigned spillFrame(unsigned tick) const {
        return startedAt?std::min((tick-startedAt)/FrameTicks,3u):0;
    }

    unsigned hudFrame(unsigned tick) const {
        if (finishedAt) return 2+(tick-finishedAt)/FrameTicks%2;
        return tick/FrameTicks%2;
    }

    unsigned label(unsigned tick) const {
        if (finishedAt) return tick-finishedAt<RewardDelay?2:0;
        return startedAt?1:0;
    }

    float textRatio(unsigned tick) const {
        if (finishedAt) return std::clamp((tick-finishedAt)/float(TextTicks),0.0f,1.0f);
        if (!startedAt) return 0;
        return std::clamp((float(tick-startedAt)-SlideTicks)/TextTicks,0.0f,1.0f);
    }

    float hudOffsetY(unsigned tick) const {
        constexpr float Offscreen=128.0f;
        if (finishedAt) {
            const float progress=std::clamp((float(tick-finishedAt)-RewardDelay)/SlideTicks,0.0f,1.0f);
            return Offscreen*std::pow(progress,3.03143f);
        }
        if (!startedAt) return Offscreen;
        const float remaining=1.0f-std::min((tick-startedAt)/float(SlideTicks),1.0f);
        const float squared=remaining*remaining;
        return Offscreen*squared*squared;
    }

private:
    static constexpr unsigned SlideTicks=30,TextTicks=90,RewardDelay=180,FrameTicks=12;
    bool available=false,spillOverlapping=false;

    static bool overlaps(Vec2 player,float radius,Vec2 center,Vec2 half) {
        const Vec2 delta=player-center;
        const Vec2 distance{std::max(std::abs(delta.x)-half.x,0.0f),std::max(std::abs(delta.y)-half.y,0.0f)};
        return dot(distance,distance)<=radius*radius;
    }
};
